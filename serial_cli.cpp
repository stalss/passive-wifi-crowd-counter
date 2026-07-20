#include "serial_cli.h"
#include "config.h"
#include "hash_table.h"
#include "sniffer.h"
#include "stats.h"
#include "channel_manager.h"
#include "reporter.h"
#include "oui_lookup.h"
#include <Arduino.h>
#include <string.h>
#include <stdlib.h>

// ============================================================
//  Serial CLI Implementation
// ============================================================

static char     g_buf[CLI_BUFFER_SIZE];
static uint8_t  g_bufIdx = 0;
static bool     g_ledEnabled = true;

// ── Helper: trim whitespace ────────────────────────────────

static char* trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' ||
                       *end == '\n' || *end == '\r'))
        *end-- = '\0';
    return s;
}

// ── Helper: format uptime ──────────────────────────────────

static void fmtUptime(unsigned long ms, char *buf, uint8_t len) {
    unsigned long sec  = ms / 1000;
    unsigned long mins = sec / 60;
    unsigned long hrs  = mins / 60;
    snprintf(buf, len, "%02lu:%02lu:%02lu",
             hrs, mins % 60, sec % 60);
}

// ── Command: help ──────────────────────────────────────────

static void cmdHelp() {
    Serial.println();
    Serial.println("  Available commands:");
    Serial.println("  ─────────────────────────────────────");
    Serial.println("  help               Show this message");
    Serial.println("  count              Current crowd count");
    Serial.println("  peak               Peak count since boot");
    Serial.println("  avg                Rolling average");
    Serial.println("  status             Full status report");
    Serial.println("  channels           Per-channel hit counts");
    Serial.println("  vendors            Device vendor breakdown");
    Serial.println("  rssi [value]       Get/set RSSI threshold");
    Serial.println("  expiry [seconds]   Get/set MAC expiry");
    Serial.println("  dump               List all tracked MACs");
    Serial.println("  reset              Clear all devices");
    Serial.println("  csv                Toggle CSV output mode");
    Serial.println("  pause              Pause scanning");
    Serial.println("  resume             Resume scanning");
    Serial.println("  led                Toggle LED heartbeat");
    Serial.println();
}

// ── Command: count ─────────────────────────────────────────

static void cmdCount() {
    Serial.print("[count] ");
    Serial.print(hashTableCount());
    Serial.print(" / ");
    Serial.print(MAX_TRACKED_DEVICES);
    Serial.println(" devices");
}

// ── Command: peak ──────────────────────────────────────────

static void cmdPeak() {
    Serial.print("[peak] ");
    Serial.print(statsPeak());
    Serial.println(" devices (since boot)");
}

// ── Command: avg ───────────────────────────────────────────

static void cmdAvg() {
    Serial.print("[avg] ");
    Serial.print(statsRollingAvg());
    Serial.println(" devices (rolling)");
}

// ── Command: status ────────────────────────────────────────

static void cmdStatus() {
    char uptime[12];
    fmtUptime(millis(), uptime, sizeof(uptime));

    Serial.println();
    Serial.println("  ┌──────────────────────────────────┐");
    Serial.println("  │      Crowd Counter  Status        │");
    Serial.println("  ├──────────────────────────────────┤");

    Serial.print("  │  Uptime      : "); Serial.print(uptime);
    Serial.println("              │");

    Serial.print("  │  Count       : "); Serial.print(hashTableCount());
    Serial.print(" / "); Serial.print(MAX_TRACKED_DEVICES);
    Serial.println("            │");

    Serial.print("  │  Peak        : "); Serial.print(statsPeak());
    Serial.println("                    │");

    Serial.print("  │  Avg         : "); Serial.print(statsRollingAvg());
    Serial.println("                    │");

    Serial.print("  │  RSSI thresh : "); Serial.print(snifferGetRssiThreshold());
    Serial.print(" dBm             │");

    Serial.print("  │  Channel     : "); Serial.print(channelManagerCurrent());
    Serial.println("                     │");

    Serial.print("  │  Paused      : ");
    Serial.print(snifferIsPaused() ? "YES" : "NO");
    Serial.println("                   │");

    Serial.print("  │  CSV mode    : ");
    Serial.print(reporterIsCsvMode() ? "YES" : "NO");
    Serial.println("                   │");

    Serial.println("  └──────────────────────────────────┘");
    Serial.println();
}

// ── Command: channels ──────────────────────────────────────

static void cmdChannels() {
    uint16_t *hits = channelManagerHits();
    Serial.println("  Per-channel hit distribution:");
    for (uint8_t ch = CHANNEL_MIN; ch <= CHANNEL_MAX; ch++) {
        Serial.print("    ch ");
        if (ch < 10) Serial.print(' ');
        Serial.print(ch);
        Serial.print(" : ");
        Serial.print(hits[ch]);
        // Simple bar chart
        uint8_t bar = hits[ch] / 5;
        if (bar > 40) bar = 40;
        Serial.print("  ");
        for (uint8_t i = 0; i < bar; i++) Serial.print('#');
        Serial.println();
    }
}

// ── Command: vendors ───────────────────────────────────────

static void cmdVendors() {
#if ENABLE_OUI_LOOKUP
    DeviceSlot *slots = hashTableSlots();
    uint16_t size     = hashTableSize();

    uint16_t counts[4] = {0};   // indexed by VendorType
    const char* names[4] = {"Unknown", "Phone", "Laptop", "IoT"};

    for (uint16_t i = 0; i < size; i++) {
        if (!slots[i].occupied) continue;
        const OuiEntry *e = ouiLookup(slots[i].mac);
        if (e) counts[e->type]++;
        else   counts[VENDOR_UNKNOWN]++;
    }

    Serial.println("  Device vendor breakdown:");
    for (uint8_t t = 0; t < 4; t++) {
        if (counts[t] == 0) continue;
        Serial.print("    ");
        Serial.print(names[t]);
        Serial.print(" : ");
        Serial.println(counts[t]);
    }
#else
    Serial.println("  [OUI lookup disabled in config.h]");
#endif
}

// ── Command: dump ──────────────────────────────────────────

static void cmdDump() {
    DeviceSlot *slots = hashTableSlots();
    uint16_t size     = hashTableSize();
    unsigned long now = millis();

    Serial.println();
    Serial.println("  MAC                 RSSI    Age(s)  Vendor");
    Serial.println("  ──────────────────  ──────  ──────  ──────────");

    uint16_t count = 0;
    for (uint16_t i = 0; i < size; i++) {
        if (!slots[i].occupied) continue;

        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 slots[i].mac[0], slots[i].mac[1], slots[i].mac[2],
                 slots[i].mac[3], slots[i].mac[4], slots[i].mac[5]);

        unsigned long ageSec = (now - slots[i].lastSeenMs) / 1000;

        Serial.print("  ");
        Serial.print(macStr);
        Serial.print("   ");
        if (slots[i].rssi > -10) Serial.print(' ');
        Serial.print(slots[i].rssi);
        Serial.print(" dBm   ");
        if (ageSec < 10)  Serial.print(' ');
        if (ageSec < 100) Serial.print(' ');
        Serial.print(ageSec);
        Serial.print("s    ");

#if ENABLE_OUI_LOOKUP
        const OuiEntry *e = ouiLookup(slots[i].mac);
        Serial.print(e ? e->name : "-");
#endif

        Serial.println();
        count++;
    }

    Serial.print("  ──────────────────────────────────────");
    Serial.print("─── Total: ");
    Serial.print(count);
    Serial.println();
}

// ── Command: reset ─────────────────────────────────────────

static void cmdReset() {
    hashTableReset();
    statsReset();
    channelManagerResetHits();
    Serial.println("[reset] All devices cleared.");
}

// ── Command: rssi ──────────────────────────────────────────

static void cmdRssi(char *args) {
    if (*args == '\0') {
        Serial.print("[rssi] Current threshold: ");
        Serial.print(snifferGetRssiThreshold());
        Serial.println(" dBm");
        return;
    }
    int val = atoi(args);
    if (val < RSSI_THRESHOLD_MIN || val > RSSI_THRESHOLD_MAX) {
        Serial.print("[rssi] Must be between ");
        Serial.print(RSSI_THRESHOLD_MIN);
        Serial.print(" and ");
        Serial.print(RSSI_THRESHOLD_MAX);
        Serial.println(" dBm");
        return;
    }
    snifferSetRssiThreshold(val);
    Serial.print("[rssi] Threshold set to ");
    Serial.print(val);
    Serial.println(" dBm");
}

// ── Command: expiry ────────────────────────────────────────

// Note: we use a global in the main .ino to communicate
// the expiry value.  For simplicity, we declare it extern
// here and define it in the .ino.
extern unsigned long g_macExpiryMs;

static void cmdExpiry(char *args) {
    if (*args == '\0') {
        Serial.print("[expiry] Current: ");
        Serial.print(g_macExpiryMs / 1000);
        Serial.println(" seconds");
        return;
    }
    unsigned long sec = atol(args);
    if (sec < 10 || sec > 3600) {
        Serial.println("[expiry] Must be between 10 and 3600 seconds");
        return;
    }
    g_macExpiryMs = sec * 1000;
    Serial.print("[expiry] Set to ");
    Serial.print(sec);
    Serial.println(" seconds");
}

// ── Command: csv ───────────────────────────────────────────

static void cmdCsv() {
    bool mode = !reporterIsCsvMode();
    reporterSetCsvMode(mode);
    Serial.print("[csv] Output mode: ");
    Serial.println(mode ? "CSV" : "Human-readable");
}

// ── Command: pause / resume ────────────────────────────────

static void cmdPause() {
    snifferPause();
    Serial.println("[pause] Scanning paused.");
}

static void cmdResume() {
    snifferResume();
    Serial.println("[resume] Scanning resumed.");
}

// ── Command: led ───────────────────────────────────────────

static void cmdLed() {
    g_ledEnabled = !g_ledEnabled;
    if (!g_ledEnabled) digitalWrite(LED_PIN, LOW);
    Serial.print("[led] Heartbeat: ");
    Serial.println(g_ledEnabled ? "ON" : "OFF");
}

// ── Parse and dispatch ─────────────────────────────────────

static void cliExecute(char *line) {
    char *cmd = trim(line);
    if (*cmd == '\0') return;

    // Split command and arguments at first space
    char *args = cmd;
    while (*args && *args != ' ') args++;
    if (*args == ' ') { *args = '\0'; args++; }
    else              { args = cmd + strlen(cmd); }  // no args

    // Dispatch
    if      (strcmp(cmd, "help")    == 0) cmdHelp();
    else if (strcmp(cmd, "count")   == 0) cmdCount();
    else if (strcmp(cmd, "peak")    == 0) cmdPeak();
    else if (strcmp(cmd, "avg")     == 0) cmdAvg();
    else if (strcmp(cmd, "status")  == 0) cmdStatus();
    else if (strcmp(cmd, "channels") == 0) cmdChannels();
    else if (strcmp(cmd, "vendors") == 0) cmdVendors();
    else if (strcmp(cmd, "dump")    == 0) cmdDump();
    else if (strcmp(cmd, "reset")   == 0) cmdReset();
    else if (strcmp(cmd, "rssi")    == 0) cmdRssi(args);
    else if (strcmp(cmd, "expiry")  == 0) cmdExpiry(args);
    else if (strcmp(cmd, "csv")     == 0) cmdCsv();
    else if (strcmp(cmd, "pause")   == 0) cmdPause();
    else if (strcmp(cmd, "resume")  == 0) cmdResume();
    else if (strcmp(cmd, "led")     == 0) cmdLed();
    else {
        Serial.print("[cli] Unknown command: ");
        Serial.println(cmd);
        Serial.println("       Type 'help' for a list of commands.");
    }
}

// ── Public API ─────────────────────────────────────────────

void cliInit() {
    g_bufIdx = 0;
    memset(g_buf, 0, sizeof(g_buf));
    Serial.println("  Type 'help' for a list of commands.");
}

bool cliPoll() {
    if (!ENABLE_SERIAL_CLI) return false;

    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (g_bufIdx > 0) {
                g_buf[g_bufIdx] = '\0';
                cliExecute(g_buf);
                g_bufIdx = 0;
                memset(g_buf, 0, sizeof(g_buf));
                Serial.print(CLI_PROMPT);
            }
        } else if (g_bufIdx < CLI_BUFFER_SIZE - 1) {
            g_buf[g_bufIdx++] = c;
        }
    }
    return false;
}

// ── LED heartbeat (called from main loop) ──────────────────

void cliLedHeartbeat() {
    if (!g_ledEnabled || LED_PIN < 0) return;

    static unsigned long lastBlink = 0;

    if (millis() - lastBlink >= 1000UL) {
        lastBlink = millis();
        digitalWrite(LED_PIN, HIGH);
    }
    if (millis() - lastBlink > 50UL) {
        digitalWrite(LED_PIN, LOW);
    }
}
