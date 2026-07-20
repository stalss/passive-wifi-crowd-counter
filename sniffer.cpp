#include "sniffer.h"
#include "config.h"
#include "hash_table.h"
#include "channel_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>

// ============================================================
//  Sniffer Implementation
// ============================================================

static int       g_rssiThreshold = RSSI_THRESHOLD_DEFAULT;
static volatile bool g_paused    = false;

// ── The ISR callback ───────────────────────────────────────
//
//  802.11 Management Frame Header:
//
//    Offset  Bytes  Field
//    ------  -----  ────────────────────────────────────────
//     0       2     Frame Control
//                     Byte 0 bits[7:4] = Subtype
//                     Byte 0 bits[3:2] = Type (0 = Mgmt)
//                     Byte 0 bits[1:0] = Protocol version
//     2       2     Duration / ID
//     4       6     Address 1 — Destination
//    10       6     Address 2 — Source  ← extract this
//    16       6     Address 3 — BSSID
//
//  Probe Request subtype = 0x04 → Frame Control byte 0 = 0x40.
//
//  RSSI comes from the ESP32 driver metadata, NOT from the
//  802.11 frame itself.

static void IRAM_ATTR snifferCallback(void *buf,
                                      wifi_promiscuous_pkt_type_t type)
{
    // Quick bail-out if paused
    if (g_paused) return;

    // Only Management frames carry Probe Requests
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t *pkt = (wifi_promiscuous_pkt_t *)buf;
    const uint8_t *frame = pkt->payload;

    // ── Frame Control ─────────────────────────────────────
    //  Byte 0 bits[7:4] = subtype (4 bits, values 0–15).
    //  Probe Request  = subtype 0x04.
    //  Auth           = subtype 0x0B.
    //  Assoc Req      = subtype 0x00.
    //  Reassoc Req    = subtype 0x02.
    //
    //  We accept only Probe Request (0x04) for the cleanest
    //  signal — these are the most frequent and reliable
    //  indicators of a nearby device with Wi-Fi enabled.
    // ──────────────────────────────────────────────────────
    uint8_t subtype = (frame[0] >> 4) & 0x0F;

    if (subtype != 0x04) return;

    // ── Source MAC ────────────────────────────────────────
    //  Address 2 (Source Address) is at byte offset 10 in
    //  the fixed-length Management frame header.
    // ──────────────────────────────────────────────────────
    const uint8_t *srcMac = &frame[10];

    // ── RSSI ─────────────────────────────────────────────
    //  The radio's receive-control metadata, attached by
    //  the ESP32 driver — NOT part of the 802.11 frame.
    // ──────────────────────────────────────────────────────
    int8_t rssi = pkt->rx_ctrl.rssi;

    if (rssi < g_rssiThreshold) return;

    // ── Channel hit counter ──────────────────────────────
    uint8_t ch = pkt->rx_ctrl.channel;
    if (ch <= CHANNEL_MAX) {
        channelManagerHits()[ch]++;
    }

    // ── Hash table upsert ────────────────────────────────
    bool isNew = hashTableUpsert(srcMac, rssi);

    // LED flash on new device (leave on; loop turns off)
    if (isNew && LED_PIN >= 0) {
        digitalWrite(LED_PIN, HIGH);
    }
}

// ── Public API ─────────────────────────────────────────────

void snifferInit() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(snifferCallback);

    // Accept all management frames (not just probe reqs at
    // the hardware filter level — we filter in the callback
    // for maximum control).
    esp_wifi_set_promiscuous_filter(false);

    esp_wifi_set_channel(channelManagerCurrent(),
                         WIFI_SECOND_CHAN_NONE);
}

int snifferGetRssiThreshold() {
    return g_rssiThreshold;
}

void snifferSetRssiThreshold(int value) {
    g_rssiThreshold = value;
}

void snifferPause() {
    g_paused = true;
}

void snifferResume() {
    g_paused = false;
}

bool snifferIsPaused() {
    return g_paused;
}
