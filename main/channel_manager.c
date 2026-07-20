#include "channel_manager.h"
#include "config.h"

#include "esp_wifi.h"
#include "esp_log.h"

#include <string.h>

/* ================================================================
 *  Channel Hopper — Non-blocking, cycles 1–13
 * ================================================================ */

static const char *TAG = "channel_mgr";

static uint8_t       g_channel    = CHANNEL_MIN;
static unsigned long g_lastHopMs  = 0;
static uint16_t      g_hits[CHANNEL_MAX + 1];

void channelManagerInit(void) {
    memset(g_hits, 0, sizeof(g_hits));
    g_lastHopMs = millis();
    g_channel   = CHANNEL_MIN;
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);
    ESP_LOGI(TAG, "Channel manager initialised (ch %d)", g_channel);
}

void channelManagerHop(void) {
    if (millis() - g_lastHopMs < CHANNEL_HOP_MS) return;
    g_lastHopMs = millis();

    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);

    g_channel++;
    if (g_channel > CHANNEL_MAX) g_channel = CHANNEL_MIN;
}

uint8_t channelManagerCurrent(void) {
    return g_channel;
}

uint16_t *channelManagerHits(void) {
    return g_hits;
}

void channelManagerResetHits(void) {
    memset(g_hits, 0, sizeof(g_hits));
}
