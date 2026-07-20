#include "channel_manager.h"
#include "config.h"
#include <esp_wifi.h>

// ============================================================
//  Channel Manager Implementation
// ============================================================

static uint8_t  g_channel       = CHANNEL_MIN;
static unsigned long g_lastHopMs = 0;
static uint16_t g_hits[CHANNEL_MAX + 1];  // index 0 unused

void channelManagerInit() {
    memset(g_hits, 0, sizeof(g_hits));
    g_lastHopMs = millis();
    g_channel   = CHANNEL_MIN;
    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);
}

void channelManagerHop() {
    if (millis() - g_lastHopMs < CHANNEL_HOP_MS) return;
    g_lastHopMs = millis();

    esp_wifi_set_channel(g_channel, WIFI_SECOND_CHAN_NONE);

    g_channel++;
    if (g_channel > CHANNEL_MAX) g_channel = CHANNEL_MIN;
}

uint8_t channelManagerCurrent() {
    return g_channel;
}

uint16_t* channelManagerHits() {
    return g_hits;
}

void channelManagerResetHits() {
    memset(g_hits, 0, sizeof(g_hits));
}
