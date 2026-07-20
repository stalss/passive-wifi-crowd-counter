/* ================================================================
 *  Passive WiFi Crowd Counter  —  ESP32  (ESP-IDF + CMake)
 * ================================================================
 *
 *  v3.0 — Pure ESP-IDF C build, modular architecture
 *
 *  Modules:
 *    config.h            All tunable constants
 *    types.h             Shared data structures
 *    hash_table.c/.h     MAC deduplication (ISR-safe)
 *    sniffer.c/.h        802.11 packet parsing callback
 *    channel_manager.c/.h  Non-blocking channel hopper
 *    stats.c/.h          Rolling average & peak tracker
 *    serial_cli.c/.h     Interactive CLI (esp_console)
 *    reporter.c/.h       Formatted serial output
 *    oui_lookup.c/.h     MAC vendor prefix matching
 *
 *  Build:
 *    idf.py build
 *    idf.py -p /dev/ttyUSB0 flash monitor
 *
 *  CLI: type 'help' in the monitor for commands.
 *
 * ================================================================ */

#include "config.h"
#include "types.h"
#include "hash_table.h"
#include "sniffer.h"
#include "channel_manager.h"
#include "stats.h"
#include "serial_cli.h"
#include "reporter.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_console.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include <stdio.h>
#include <string.h>

/* ================================================================
 *  Global state shared with serial_cli.c
 * ================================================================ */

unsigned long g_macExpiryMs = MAC_EXPIRY_MS;

static const char *TAG = "main";

/* ================================================================
 *  Channel Hopping Task
 * ================================================================ */

static void channelTask(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "Channel hopper task started");

    for (;;) {
        channelManagerHop();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ================================================================
 *  Expiry Task — evict stale MACs once per second
 * ================================================================ */

static void expireTask(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "Expiry task started");

    for (;;) {
        hashTableExpire(millis(), g_macExpiryMs);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ================================================================
 *  Report Task — print stats every REPORT_INTERVAL_MS
 * ================================================================ */

static void reportTask(void *arg) {
    (void)arg;
    ESP_LOGI(TAG, "Report task started");

    for (;;) {
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

        reporterPrint(&s);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* ================================================================
 *  LED Heartbeat Task — 1 Hz blink
 * ================================================================ */

static void ledTask(void *arg) {
    (void)arg;

    for (;;) {
        cliLedHeartbeat();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* ================================================================
 *  CLI REPL Task — runs esp_console REPL
 * ================================================================ */

static void cliTask(void *arg) {
    (void)arg;

    /* Configure UART for esp_console */
    esp_console_dev_uart_config_t uart_config =
        ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

    esp_console_config_t console_config = {
        .max_cmdline_length = CLI_BUFFER_SIZE,
        .max_cmdline_args   = 8,
        .hint_color         = 36,   /* cyan */
        .hint_bold          = 1,
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Register CLI commands */
    cliInit();

    /* Start REPL */
    esp_console_repl_config_t repl_config =
        ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.task_stack_size = TASK_STACK_SIZE_CLI;
    repl_config.task_priority   = TASK_PRIO_CLI;

    esp_console_repl_t *repl = NULL;
    esp_console_new_repl_uart(&uart_config, &repl_config, &repl);

    /* REPL runs forever in this task — never returns */
    ESP_LOGI(TAG, "CLI ready — type 'help' for commands");
}

/* ================================================================
 *  app_main — ESP-IDF entry point
 * ================================================================ */

void app_main(void) {
    /* ── NVS init (required by WiFi driver) ──────────────── */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  Passive WiFi Crowd Counter  v3.0");
    ESP_LOGI(TAG, "  ESP-IDF CMake Build");
    ESP_LOGI(TAG, "==============================================");
    ESP_LOGI(TAG, "  Channels        : %d – %d", CHANNEL_MIN, CHANNEL_MAX);
    ESP_LOGI(TAG, "  Dwell / channel : %d ms", CHANNEL_HOP_MS);
    ESP_LOGI(TAG, "  RSSI threshold  : %d dBm", RSSI_THRESHOLD_DEFAULT);
    ESP_LOGI(TAG, "  Expiry timeout  : %lu min", MAC_EXPIRY_MS / 60000);
    ESP_LOGI(TAG, "  Max devices     : %d", MAX_TRACKED_DEVICES);
    ESP_LOGI(TAG, "  Hash table      : %d buckets", HASH_TABLE_SIZE);
    ESP_LOGI(TAG, "  Avg window      : %d", AVG_WINDOW);
    ESP_LOGI(TAG, "  Report every    : %lu s", REPORT_INTERVAL_MS / 1000);
    ESP_LOGI(TAG, "==============================================");

    /* ── Init subsystems ─────────────────────────────────── */
    hashTableInit();
    statsInit();
    channelManagerInit();
    reporterInit();
    snifferInit();          /* must come after channelManagerInit */

    /* ── Create FreeRTOS tasks ───────────────────────────── */

    xTaskCreate(channelTask, "channel",  TASK_STACK_SIZE_CHANNEL, NULL,
                TASK_PRIO_CHANNEL, NULL);

    xTaskCreate(expireTask, "expire",    TASK_STACK_SIZE_EXPIRE, NULL,
                TASK_PRIO_EXPIRE, NULL);

    xTaskCreate(reportTask, "report",    TASK_STACK_SIZE_REPORT, NULL,
                TASK_PRIO_REPORT, NULL);

    xTaskCreate(ledTask,    "led",       2048, NULL, 1, NULL);

    xTaskCreate(cliTask,    "cli",       TASK_STACK_SIZE_CLI, NULL,
                TASK_PRIO_CLI, NULL);

    ESP_LOGI(TAG, "All tasks started — entering idle loop");
}
