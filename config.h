#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  Passive WiFi Crowd Counter — Configuration
// ============================================================
//
//  Edit these constants to tune the device for your
//  environment.  All runtime-adjustable parameters can also
//  be changed via the serial CLI (see serial_cli.h).
//
// ============================================================

// ── Wi-Fi Scanning ─────────────────────────────────────────

// Channels to scan (1–13 covers all worldwide 2.4 GHz bands).
#define CHANNEL_MIN              1
#define CHANNEL_MAX              13

// Milliseconds to dwell on each channel before hopping.
// Lower = faster sweep, higher = more captures per channel.
#define CHANNEL_HOP_MS           200

// ── Signal Filter ──────────────────────────────────────────

// Default RSSI threshold (dBm).  Packets weaker than this
// are ignored.  Typical: -70 (one room), -60 (close),
// -80 (wide area).
#define RSSI_THRESHOLD_DEFAULT   (-70)

// Allowed range for runtime adjustment via CLI.
#define RSSI_THRESHOLD_MIN       (-90)
#define RSSI_THRESHOLD_MAX       (-30)

// ── Device Tracking ────────────────────────────────────────

// How long (ms) before a silent MAC is evicted.
#define MAC_EXPIRY_MS            300000UL  // 5 minutes

// Hard cap on unique MACs to protect RAM.
#define MAX_TRACKED_DEVICES      200

// Hash table bucket count.  Must be prime and >= 1.1× the
// max device count for good open-addressing distribution.
#define HASH_TABLE_SIZE          211

// ── Statistics & Reporting ─────────────────────────────────

// Number of samples for the rolling average (smooths jitter).
#define AVG_WINDOW               6

// Milliseconds between serial report prints.
#define REPORT_INTERVAL_MS       5000UL

// ── Hardware ───────────────────────────────────────────────

// Built-in LED GPIO (GPIO 2 on most DevKit boards).
// Set to -1 to disable.
#define LED_PIN                  2

// Serial monitor baud rate.
#define SERIAL_BAUD              115200

// ── Serial CLI ─────────────────────────────────────────────

#define ENABLE_SERIAL_CLI        1       // 1 = on, 0 = off
#define CLI_BUFFER_SIZE          64
#define CLI_PROMPT               "crowd> "

// ── CSV Logging Mode ───────────────────────────────────────

// When enabled, the serial output switches to CSV format
// for easy data collection (pipe to a .csv file).
#define ENABLE_CSV_LOGGING       1       // 1 = on, 0 = off

// ── OUI Vendor Lookup ──────────────────────────────────────

// Match the first 3 bytes of a MAC against known OUI
// prefixes to categorise devices (phone, laptop, IoT, etc.).
#define ENABLE_OUI_LOOKUP        1       // 1 = on, 0 = off
#define OUI_TABLE_SIZE           12

// ── Optional: Web Server ───────────────────────────────────
//
//  When enabled, the ESP32 runs a lightweight HTTP server
//  alongside promiscuous mode (APSTA mode) and exposes a
//  JSON API at  http://192.168.4.1/api/count
//
//  NOTE: This may slightly reduce capture performance
//  because the radio time-slices between AP and STA modes.
//
#define ENABLE_WEB_SERVER        0       // 1 = on, 0 = off
#define WEB_SERVER_PORT          80
#define AP_SSID                  "CrowdCounter"
#define AP_PASSWORD              ""      // empty = open AP

#endif // CONFIG_H
