#include "serial_cli.h"
#include "config.h"
#include "hash_table.h"
#include "sniffer.h"
#include "stats.h"
#include "channel_manager.h"
#include "reporter.h"
#include "oui_lookup.h"

#include "esp_log.h"
#include "esp_console.h"
#include "driver/gpio.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  Serial CLI — ESP-IDF esp_console REPL
 *
 *  Provides an interactive command-line interface over UART
 *  with tab completion, command history, and coloured output.
 * ================================================================ */

static const char *TAG = "cli";

/* ── Extern: mac expiry (set from app_main) ──────────────────── */
extern unsigned long g_macExpiryMs;

/* ── LED state ───────────────────────────────────────────────── */
static int g_ledEnabled = 1;

/* ── Helper: format uptime ───────────────────────────────────── */
static void fmtUptime(unsigned long ms, char *buf, int len) {
    unsigned long sec  = ms / 1000;
    unsigned long mins = sec / 60;
    unsigned long hrs  = mins / 60;
    snprintf(buf, len, "%02lu:%02lu:%02lu",
             hrs, mins % 60, sec % 60);
}

/* ── Helper: trim whitespace ─────────────────────────────────── */
static char *trim(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    char *end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' ||
                       *end == '\n' || *end == '\r'))
        *end-- = '\0';
    return s;
}

/* ================================================================
 *  CLI Commands
 * ================================================================ */

/* help */
static int cmdHelp(int argc, char **argv) {
    printf("\n");
    printf("  Available commands:\n");
    printf("  ─────────────────────────────────────\n");
    printf("  help               Show this message\n");
    printf("  count              Current crowd count\n");
    printf("  peak               Peak count since boot\n");
    printf("  avg                Rolling average\n");
    printf("  status             Full status report\n");
    printf("  channels           Per-channel hit counts\n");
    printf("  vendors            Device vendor breakdown\n");
    printf("  rssi [value]       Get/set RSSI threshold\n");
    printf("  expiry [seconds]   Get/set MAC expiry\n");
    printf("  dump               List all tracked MACs\n");
    printf("  reset              Clear all devices\n");
    printf("  csv                Toggle CSV output\n");
    printf("  pause              Pause scanning\n");
    printf("  resume             Resume scanning\n");
    printf("  led                Toggle LED heartbeat\n");
    printf("\n");
    return 0;
}

/* count */
static int cmdCount(int argc, char **argv) {
    printf("[count] %u / %u devices\n",
           hashTableCount(), hashTableCapacity());
    return 0;
}

/* peak */
static int cmdPeak(int argc, char **argv) {
    printf("[peak] %u devices (since boot)\n", statsPeak());
    return 0;
}

/* avg */
static int cmdAvg(int argc, char **argv) {
    printf("[avg] %u devices (rolling)\n", statsRollingAvg());
    return 0;
}

/* status */
static int cmdStatus(int argc, char **argv) {
    char uptime[12];
    fmtUptime(millis(), uptime, sizeof(uptime));

    printf("\n");
    printf("  ┌──────────────────────────────────┐\n");
    printf("  │      Crowd Counter  Status        │\n");
    printf("  ├──────────────────────────────────┤\n");
    printf("  │  Uptime      : %-18s│\n", uptime);
    printf("  │  Count       : %u / %-13u│\n",
           hashTableCount(), hashTableCapacity());
    printf("  │  Peak        : %-18u│\n", statsPeak());
    printf("  │  Avg         : %-18u│\n", statsRollingAvg());
    printf("  │  RSSI thresh : %-14d dBm  │\n",
           snifferGetRssiThreshold());
    printf("  │  Channel     : %-18u│\n",
           channelManagerCurrent());
    printf("  │  Paused      : %-18s│\n",
           snifferIsPaused() ? "YES" : "NO");
    printf("  │  CSV mode    : %-18s│\n",
           reporterIsCsvMode() ? "YES" : "NO");
    printf("  │  LED         : %-18s│\n",
           g_ledEnabled ? "ON" : "OFF");
    printf("  └──────────────────────────────────┘\n");
    printf("\n");
    return 0;
}

/* channels */
static int cmdChannels(int argc, char **argv) {
    uint16_t *hits = channelManagerHits();
    printf("  Per-channel hit distribution:\n");
    for (uint8_t ch = CHANNEL_MIN; ch <= CHANNEL_MAX; ch++) {
        printf("    ch %2u : %5u  ", ch, hits[ch]);
        uint8_t bar = hits[ch] / 5;
        if (bar > 40) bar = 40;
        for (uint8_t i = 0; i < bar; i++) printf("#");
        printf("\n");
    }
    return 0;
}

/* vendors */
static int cmdVendors(int argc, char **argv) {
#if ENABLE_OUI_LOOKUP
    DeviceSlot *slots = hashTableSlots();
    uint16_t size = hashTableSize();
    uint16_t counts[4] = {0};
    const char *names[] = {"Unknown", "Phone", "Laptop", "IoT"};

    for (uint16_t i = 0; i < size; i++) {
        if (!slots[i].occupied) continue;
        const OuiEntry *e = ouiLookup(slots[i].mac);
        if (e) counts[e->type]++;
        else   counts[VENDOR_UNKNOWN]++;
    }

    printf("  Device vendor breakdown:\n");
    for (int t = 0; t < 4; t++) {
        if (counts[t] == 0) continue;
        printf("    %-10s : %u\n", names[t], counts[t]);
    }
#else
    printf("  [OUI lookup disabled in config.h]\n");
#endif
    return 0;
}

/* dump */
static int cmdDump(int argc, char **argv) {
    DeviceSlot *slots = hashTableSlots();
    uint16_t size = hashTableSize();
    unsigned long now = millis();

    printf("\n");
    printf("  MAC                 RSSI    Age(s)  Vendor\n");
    printf("  ──────────────────  ──────  ──────  ──────────\n");

    uint16_t count = 0;
    for (uint16_t i = 0; i < size; i++) {
        if (!slots[i].occupied) continue;

        unsigned long ageSec = (now - slots[i].lastSeenMs) / 1000;

        printf("  %02X:%02X:%02X:%02X:%02X:%02X   %3d dBm   %4lus    ",
               slots[i].mac[0], slots[i].mac[1], slots[i].mac[2],
               slots[i].mac[3], slots[i].mac[4], slots[i].mac[5],
               slots[i].rssi, ageSec);

#if ENABLE_OUI_LOOKUP
        const OuiEntry *e = ouiLookup(slots[i].mac);
        printf("%s", e ? e->name : "-");
#endif
        printf("\n");
        count++;
    }

    printf("  ──────────────────────────────────────"
           "─── Total: %u\n", count);
    return 0;
}

/* reset */
static int cmdReset(int argc, char **argv) {
    hashTableReset();
    statsReset();
    channelManagerResetHits();
    printf("[reset] All devices cleared.\n");
    return 0;
}

/* rssi */
static int cmdRssi(int argc, char **argv) {
    if (argc < 2) {
        printf("[rssi] Current threshold: %d dBm\n",
               snifferGetRssiThreshold());
        return 0;
    }
    int val = atoi(argv[1]);
    if (val < RSSI_THRESHOLD_MIN || val > RSSI_THRESHOLD_MAX) {
        printf("[rssi] Must be between %d and %d dBm\n",
               RSSI_THRESHOLD_MIN, RSSI_THRESHOLD_MAX);
        return 1;
    }
    snifferSetRssiThreshold(val);
    printf("[rssi] Threshold set to %d dBm\n", val);
    return 0;
}

/* expiry */
static int cmdExpiry(int argc, char **argv) {
    if (argc < 2) {
        printf("[expiry] Current: %lu seconds\n", g_macExpiryMs / 1000);
        return 0;
    }
    unsigned long sec = (unsigned long)atol(argv[1]);
    if (sec < 10 || sec > 3600) {
        printf("[expiry] Must be between 10 and 3600 seconds\n");
        return 1;
    }
    g_macExpiryMs = sec * 1000;
    printf("[expiry] Set to %lu seconds\n", sec);
    return 0;
}

/* csv */
static int cmdCsv(int argc, char **argv) {
    int mode = !reporterIsCsvMode();
    reporterSetCsvMode(mode);
    printf("[csv] Output mode: %s\n", mode ? "CSV" : "Human-readable");
    return 0;
}

/* pause */
static int cmdPause(int argc, char **argv) {
    snifferPause();
    printf("[pause] Scanning paused.\n");
    return 0;
}

/* resume */
static int cmdResume(int argc, char **argv) {
    snifferResume();
    printf("[resume] Scanning resumed.\n");
    return 0;
}

/* led */
static int cmdLed(int argc, char **argv) {
    g_ledEnabled = !g_ledEnabled;
    if (!g_ledEnabled) gpio_set_level(LED_PIN, 0);
    printf("[led] Heartbeat: %s\n", g_ledEnabled ? "ON" : "OFF");
    return 0;
}

/* ================================================================
 *  Register all commands with esp_console
 * ================================================================ */

void cliInit(void) {
    esp_console_cmd_t cmd;

    cmd.command = "help";
    cmd.help    = "Show available commands";
    cmd.hint    = NULL;
    cmd.func    = cmdHelp;
    esp_console_cmd_register(&cmd);

    cmd.command = "count";
    cmd.help    = "Current crowd count";
    cmd.func    = cmdCount;
    esp_console_cmd_register(&cmd);

    cmd.command = "peak";
    cmd.help    = "Peak count since boot";
    cmd.func    = cmdPeak;
    esp_console_cmd_register(&cmd);

    cmd.command = "avg";
    cmd.help    = "Rolling average";
    cmd.func    = cmdAvg;
    esp_console_cmd_register(&cmd);

    cmd.command = "status";
    cmd.help    = "Full status report";
    cmd.func    = cmdStatus;
    esp_console_cmd_register(&cmd);

    cmd.command = "channels";
    cmd.help    = "Per-channel hit distribution";
    cmd.func    = cmdChannels;
    esp_console_cmd_register(&cmd);

    cmd.command = "vendors";
    cmd.help    = "Device vendor breakdown";
    cmd.func    = cmdVendors;
    esp_console_cmd_register(&cmd);

    cmd.command = "dump";
    cmd.help    = "List all tracked MACs";
    cmd.func    = cmdDump;
    esp_console_cmd_register(&cmd);

    cmd.command = "reset";
    cmd.help    = "Clear all devices";
    cmd.func    = cmdReset;
    esp_console_cmd_register(&cmd);

    cmd.command = "rssi";
    cmd.help    = "Get/set RSSI threshold";
    cmd.hint    = "[value]";
    cmd.func    = cmdRssi;
    esp_console_cmd_register(&cmd);

    cmd.command = "expiry";
    cmd.help    = "Get/set MAC expiry timeout";
    cmd.hint    = "[seconds]";
    cmd.func    = cmdExpiry;
    esp_console_cmd_register(&cmd);

    cmd.command = "csv";
    cmd.help    = "Toggle CSV output mode";
    cmd.func    = cmdCsv;
    esp_console_cmd_register(&cmd);

    cmd.command = "pause";
    cmd.help    = "Pause scanning";
    cmd.func    = cmdPause;
    esp_console_cmd_register(&cmd);

    cmd.command = "resume";
    cmd.help    = "Resume scanning";
    cmd.func    = cmdResume;
    esp_console_cmd_register(&cmd);

    cmd.command = "led";
    cmd.help    = "Toggle LED heartbeat";
    cmd.func    = cmdLed;
    esp_console_cmd_register(&cmd);

    ESP_LOGI(TAG, "CLI initialised — type 'help' for commands");
}

/* ================================================================
 *  CLI poll — called from the REPL task
 *  (esp_console handles UART I/O and line editing)
 * ================================================================ */

void cliPoll(void) {
    /* esp_console REPL runs its own loop; nothing needed here */
}

/* ================================================================
 *  LED heartbeat — 1 Hz blink with 50 ms pulse
 * ================================================================ */

void cliLedHeartbeat(void) {
    if (!g_ledEnabled || LED_PIN < 0) return;

    static unsigned long lastBlink = 0;

    if (millis() - lastBlink >= 1000UL) {
        lastBlink = millis();
        gpio_set_level(LED_PIN, 1);
    }
    if (millis() - lastBlink > 50UL) {
        gpio_set_level(LED_PIN, 0);
    }
}
