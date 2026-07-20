// ============================================================
//  Passive WiFi Crowd Counter  —  ESP32  (Arduino IDE)
// ============================================================
//
//  v3.0 — Modular build
//
//  File structure:
//    config.h                     All tunable constants
//    types.h                      Shared data structures
//    hash_table.h / .cpp          MAC deduplication (ISR-safe)
//    sniffer.h / .cpp             802.11 packet parsing
//    channel_manager.h / .cpp     Non-blocking channel hopper
//    stats.h / .cpp               Rolling average & peak
//    serial_cli.h / .cpp          Interactive serial commands
//    reporter.h / .cpp            Formatted serial output
//    oui_lookup.h / .cpp          MAC vendor prefix matching
//
//  Quick start:
//    1. Open in Arduino IDE
//    2. Select board: ESP32 Dev Module
//    3. Upload
//    4. Open Serial Monitor at 115200 baud
//    5. Type 'help' for a list of commands
//
// ============================================================

#include "config.h"
#include "types.h"
#include "hash_table.h"
#include "sniffer.h"
#include "channel_manager.h"
#include "stats.h"
#include "serial_cli.h"
#include "reporter.h"

// ============================================================
//  Global State (shared with serial_cli.cpp via extern)
// ============================================================

unsigned long g_macExpiryMs = MAC_EXPIRY_MS;

// ============================================================
//  Expire Stale Devices  (throttled to once per second)
// ============================================================

static unsigned long g_lastExpireMs = 0;

static void expireTick() {
    if (millis() - g_lastExpireMs < 1000UL) return;
    g_lastExpireMs = millis();
    hashTableExpire(millis(), g_macExpiryMs);
}

// ============================================================
//  Report Tick  —  pushes sample, builds stats, prints
// ============================================================

static void reportTick() {
    CrowdStats s;
    s.currentCount  = hashTableCount();
    s.peakCount     = statsUpdatePeak(s.currentCount);
    statsPushSample(s.currentCount);
    s.rollingAvg    = statsRollingAvg();
    s.slotsUsed     = s.currentCount;
    s.slotsMax      = hashTableCapacity();
    s.currentChannel = channelManagerCurrent();
    s.rssiThreshold  = snifferGetRssiThreshold();
    s.uptimeMs      = millis();

    reporterPrint(s);
}

// ============================================================
//  Setup
// ============================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(1000);

    // LED
    if (LED_PIN >= 0) {
        pinMode(LED_PIN, OUTPUT);
        digitalWrite(LED_PIN, LOW);
    }

    // Init subsystems
    hashTableInit();
    statsInit();
    channelManagerInit();
    reporterInit();
    snifferInit();      // must come after channelManagerInit()

    // Boot banner
    reporterBanner();

    // CLI ready
    Serial.print(CLI_PROMPT);
    cliInit();
}

// ============================================================
//  Loop
// ============================================================

void loop() {
    channelManagerHop();
    expireTick();
    reportTick();
    cliPoll();
    cliLedHeartbeat();
}
