#ifndef CONFIG_H
#define CONFIG_H

#include "esp_timer.h"

/* ================================================================
 *  Passive WiFi Crowd Counter — Configuration
 *
 *  Edit these constants to tune the device.  Runtime-adjustable
 *  parameters can also be changed via the serial CLI.
 * ================================================================ */

/* ── millis() helper ────────────────────────────────────────────── */

static inline uint32_t millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

/* ── fmtUptime() helper ─────────────────────────────────────────── */

static inline void fmtUptime(unsigned long ms, char *buf, int len) {
    unsigned long sec  = ms / 1000;
    unsigned long mins = sec / 60;
    unsigned long hrs  = mins / 60;
    snprintf(buf, len, "%02lu:%02lu:%02lu",
             hrs, mins % 60, sec % 60);
}

/* ── Wi-Fi Scanning ─────────────────────────────────────────────── */

#define CHANNEL_MIN              1
#define CHANNEL_MAX              13
#define CHANNEL_HOP_MS           200

/* ── Signal Filter ──────────────────────────────────────────────── */

#define RSSI_THRESHOLD_DEFAULT   (-70)
#define RSSI_THRESHOLD_MIN       (-90)
#define RSSI_THRESHOLD_MAX       (-30)

/* ── Device Tracking ────────────────────────────────────────────── */

#define MAC_EXPIRY_MS            300000UL
#define MAX_TRACKED_DEVICES      200
#define HASH_TABLE_SIZE          211

/* ── Statistics & Reporting ─────────────────────────────────────── */

#define AVG_WINDOW               6
#define REPORT_INTERVAL_MS       5000UL

/* ── Hardware ───────────────────────────────────────────────────── */

#define LED_PIN                  2

/* ── Serial CLI ─────────────────────────────────────────────────── */

#define CLI_BUFFER_SIZE          64

/* ── OUI Vendor Lookup ──────────────────────────────────────────── */

#define ENABLE_OUI_LOOKUP        1
#define OUI_TABLE_SIZE           12

/* ── Task Stack Sizes ───────────────────────────────────────────── */

#define TASK_STACK_SIZE_CHANNEL  4096
#define TASK_STACK_SIZE_REPORT   4096
#define TASK_STACK_SIZE_CLI      8192
#define TASK_STACK_SIZE_EXPIRE   4096

/* ── Task Priorities ────────────────────────────────────────────── */

#define TASK_PRIO_CHANNEL        5
#define TASK_PRIO_REPORT         3
#define TASK_PRIO_CLI            2
#define TASK_PRIO_EXPIRE         3

#endif /* CONFIG_H */
