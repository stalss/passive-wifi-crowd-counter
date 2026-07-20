// ============================================================
// Passive WiFi Crowd Counter — ESP32 (Arduino IDE)
// ============================================================
//
// How it works:
//   1. The ESP32 WiFi radio is placed in promiscuous (monitor)
//      mode so it can hear *all* nearby 802.11 traffic, not
//      just traffic addressed to it.
//   2. A callback fires for every raw Layer-2 frame. We parse
//      the Frame Control header to find Probe Requests — the
//      frames that phones/laptops broadcast when Wi-Fi is
//      enabled, even when they are not connected to a network.
//   3. Each unique source MAC that sends a Probe Request with
//      a strong enough RSSI is tracked. After 5 minutes of
//      silence the entry expires.
//   4. The channel is hopped every 200 ms across channels 1-13
//      to catch devices advertising on all standard WiFi
//      channels.
//   5. A live "people count" is printed to Serial every 5 s.
//
// 802.11 MAC Header Layout (fixed portion of a Management frame):
//
//   Offset  Length  Field              Notes
//   ------  ------  ----------------   -------------------------
//    0       2      Frame Control      Byte 0 bits[7:2] = subtype
//    2       2      Duration / ID
//    4       6      Address 1          Destination (ff:ff:ff:ff:ff:ff
//                                      for Probe Requests)
//   10       6      Address 2          Source Address **(our MAC)**
//   16       6      Address 3          BSSID
//
// The RSSI value is NOT inside the frame itself. It is carried
// by the ESP32 driver in a wifi_promiscuous_pkt_t metadata
// struct that is passed to the callback alongside the raw
// payload.
//
// ============================================================

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <map>
#include <string>

// ----------------------- Configuration ----------------------

#define CHANNEL_MIN            1
#define CHANNEL_MAX            13
#define CHANNEL_HOP_MS         200    // ms to stay on each channel
#define REPORT_INTERVAL_MS     5000   // ms between serial prints
#define MAC_EXPIRY_MS          300000 // 5 minutes
#define MAX_TRACKED_DEVICES    200    // hard cap to protect RAM

const int RSSI_THRESHOLD = -70;      // ignore packets weaker than this

// ----------------------- Data Structures --------------------

struct TrackedDevice {
    uint8_t       mac[6];
    int8_t        lastRssi;
    unsigned long lastSeenMs;
};

// ----------------------- Global State -----------------------

static std::map<std::string, TrackedDevice> g_devices;
static unsigned long g_lastHopMs   = 0;
static unsigned long g_lastReportMs = 0;
static uint8_t       g_channel     = CHANNEL_MIN;

// ----------------------- Helpers ----------------------------

static std::string macToString(const uint8_t *mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}

// ----------------------- Sniffer Callback -------------------
//
// Called by the ESP32 driver for every received frame while
// promiscuous mode is active.
//
// `type` tells us whether the frame is a Management, Control,
// or Data frame. We only care about Management frames.
//
// `wifi_promiscuous_pkt_t` contains:
//   - .payload  : pointer to the raw 802.11 frame bytes
//   - .rx_ctrl  : radio metadata including .rssi (signal strength)

static void IRAM_ATTR snifferCallback(void *buf,
                                      wifi_promiscuous_pkt_type_t type)
{
    // ----------------------------------------------------------
    // Step 1: Only look at Management frames.
    // ----------------------------------------------------------
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;

    // ----------------------------------------------------------
    // Step 2: Parse the Frame Control field.
    //
    // The first two bytes of any 802.11 frame are Frame Control.
    // Byte 0, bits [7:2] = Subtype (6 bits).
    // A Probe Request has subtype 0x04.
    // ----------------------------------------------------------
    const uint8_t *frame = pkt->payload;
    uint8_t frameControl0  = frame[0];
    uint8_t subtype         = (frameControl0 >> 2) & 0x3F;

    if (subtype != 0x04) return;   // not a Probe Request

    // ----------------------------------------------------------
    // Step 3: Extract the Source MAC address.
    //
    // In the fixed-length Management frame header the Source
    // Address (Address 2) starts at byte offset 10.
    // ----------------------------------------------------------
    const uint8_t *srcMac = &frame[10];

    // ----------------------------------------------------------
    // Step 4: Read the RSSI from the radio metadata.
    // ----------------------------------------------------------
    int8_t rssi = pkt->rx_ctrl.rssi;

    if (rssi < RSSI_THRESHOLD) return;

    // ----------------------------------------------------------
    // Step 5: Deduplicate — add or refresh the entry.
    // ----------------------------------------------------------
    std::string key = macToString(srcMac);
    unsigned long now = millis();

    auto it = g_devices.find(key);
    if (it != g_devices.end()) {
        it->second.lastSeenMs = now;
        it->second.lastRssi   = rssi;
    } else if ((int)g_devices.size() < MAX_TRACKED_DEVICES) {
        TrackedDevice dev;
        memcpy(dev.mac, srcMac, 6);
        dev.lastRssi    = rssi;
        dev.lastSeenMs  = now;
        g_devices[key]  = dev;
    }
    // else: at capacity — silently drop to protect heap
}

// ----------------------- Channel Hopper ---------------------
//
// Non-blocking. Moves to the next WiFi channel every
// CHANNEL_HOP_MS milliseconds.

static void hopChannel() {
    if (millis() - g_lastHopMs < CHANNEL_HOP_MS) return;
    g_lastHopMs = millis();

    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);

    g_channel++;
    if (g_channel > CHANNEL_MAX) g_channel = CHANNEL_MIN;
}

// ----------------------- Expire Old MACs --------------------
//
// Remove any device not seen for MAC_EXPIRY_MS (5 minutes).

static void expireDevices() {
    unsigned long now = millis();
    for (auto it = g_devices.begin(); it != g_devices.end(); ) {
        if (now - it->second.lastSeenMs > MAC_EXPIRY_MS)
            it = g_devices.erase(it);
        else
            ++it;
    }
}

// ----------------------- Serial Report ----------------------

static void report() {
    if (millis() - g_lastReportMs < REPORT_INTERVAL_MS) return;
    g_lastReportMs = millis();

    Serial.print("[+] Live Crowd Count: ");
    Serial.print((int)g_devices.size());
    Serial.print(" (Threshold: ");
    Serial.print(RSSI_THRESHOLD);
    Serial.println(" dBm)");
}

// ----------------------- Arduino Entry Points ---------------

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Bring WiFi up in station mode (required before promiscuous)
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    // Enter promiscuous mode and register our callback
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(snifferCallback);

    // Start on channel 1
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);

    Serial.println("============================================");
    Serial.println("  Passive WiFi Crowd Counter - Ready");
    Serial.print("  Devices tracked: 0 / ");
    Serial.println(MAX_TRACKED_DEVICES);
    Serial.print("  RSSI threshold : ");
    Serial.print(RSSI_THRESHOLD);
    Serial.println(" dBm");
    Serial.print("  Hop interval   : ");
    Serial.print(CHANNEL_HOP_MS);
    Serial.println(" ms");
    Serial.print("  Expiry timer   : ");
    Serial.print(MAC_EXPIRY_MS / 1000 / 60);
    Serial.println(" min");
    Serial.println("============================================");

    g_lastHopMs    = millis();
    g_lastReportMs = millis();
}

void loop() {
    hopChannel();
    expireDevices();
    report();
}
