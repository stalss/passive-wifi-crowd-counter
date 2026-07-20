#include "reporter.h"
#include "config.h"
#include <Arduino.h>

// ============================================================
//  Reporter Implementation
// ============================================================

static bool     g_csvMode = false;
static unsigned long g_lastReportMs = 0;

void reporterInit() {
    g_lastReportMs = millis();
    g_csvMode      = false;
}

// ── Banner (printed once at boot) ──────────────────────────

void reporterBanner() {
    Serial.println();
    Serial.println("==============================================");
    Serial.println("  Passive WiFi Crowd Counter  v3.0");
    Serial.println("  Modular Build — Serial CLI Enabled");
    Serial.println("==============================================");
    Serial.print("  Channels        : ");
    Serial.print(CHANNEL_MIN);
    Serial.print(" – ");
    Serial.println(CHANNEL_MAX);
    Serial.print("  Dwell / channel : ");
    Serial.print(CHANNEL_HOP_MS);
    Serial.println(" ms");
    Serial.print("  RSSI threshold  : ");
    Serial.print(RSSI_THRESHOLD_DEFAULT);
    Serial.println(" dBm");
    Serial.print("  Expiry timeout  : ");
    Serial.print(MAC_EXPIRY_MS / 60000);
    Serial.println(" min");
    Serial.print("  Max devices     : ");
    Serial.println(MAX_TRACKED_DEVICES);
    Serial.print("  Hash table      : ");
    Serial.print(HASH_TABLE_SIZE);
    Serial.println(" buckets");
    Serial.print("  Avg window      : ");
    Serial.println(AVG_WINDOW);
    Serial.print("  Report every    : ");
    Serial.print(REPORT_INTERVAL_MS / 1000);
    Serial.println(" s");
    Serial.println("==============================================");
    Serial.println();
}

// ── CSV header ─────────────────────────────────────────────

void reporterCsvHeader() {
    Serial.println("timestamp_ms,count,peak,avg,rssi_thresh,channel,slots_used");
}

// ── Format uptime ──────────────────────────────────────────

static void fmtUptime(unsigned long ms, char *buf, uint8_t len) {
    unsigned long sec  = ms / 1000;
    unsigned long mins = sec / 60;
    unsigned long hrs  = mins / 60;
    snprintf(buf, len, "%02lu:%02lu:%02lu",
             hrs, mins % 60, sec % 60);
}

// ── Human-readable report ──────────────────────────────────

static void printHuman(const CrowdStats &s) {
    char uptime[12];
    fmtUptime(s.uptimeMs, uptime, sizeof(uptime));

    Serial.println("--------------------------------------------------");
    Serial.print("[");
    Serial.print(uptime);
    Serial.print("]  Crowd: ");
    Serial.print(s.currentCount);
    Serial.print("  (avg: ");
    Serial.print(s.rollingAvg);
    Serial.print(", peak: ");
    Serial.print(s.peakCount);
    Serial.print(")  RSSI >= ");
    Serial.print(s.rssiThreshold);
    Serial.print(" dBm  ch: ");
    Serial.println(s.currentChannel);

    // Channel hit summary (only non-zero)
    uint16_t *hits = nullptr;  // caller fills this in externally
    Serial.print("  Slots: ");
    Serial.print(s.slotsUsed);
    Serial.print("/");
    Serial.print(s.slotsMax);
    Serial.println();
}

// ── CSV report ─────────────────────────────────────────────

static void printCsv(const CrowdStats &s) {
    Serial.print(s.uptimeMs);
    Serial.print(',');
    Serial.print(s.currentCount);
    Serial.print(',');
    Serial.print(s.peakCount);
    Serial.print(',');
    Serial.print(s.rollingAvg);
    Serial.print(',');
    Serial.print(s.rssiThreshold);
    Serial.print(',');
    Serial.print(s.currentChannel);
    Serial.print(',');
    Serial.println(s.slotsUsed);
}

// ── Public API ─────────────────────────────────────────────

bool reporterPrint(const CrowdStats &stats) {
    if (millis() - g_lastReportMs < REPORT_INTERVAL_MS) return false;
    g_lastReportMs = millis();

    if (g_csvMode) printCsv(stats);
    else           printHuman(stats);

    return true;
}

void reporterSetCsvMode(bool enabled) {
    g_csvMode = enabled;
    if (enabled) reporterCsvHeader();
}

bool reporterIsCsvMode() {
    return g_csvMode;
}
