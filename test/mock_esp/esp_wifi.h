/* Mock ESP WiFi */
#ifndef MOCK_ESP_WIFI_H
#define MOCK_ESP_WIFI_H
#include <stdint.h>

#define WIFI_SECOND_CHAN_NONE 0

typedef enum { ESP_OK = 0 } esp_err_t;

static inline esp_err_t esp_wifi_set_channel(uint8_t ch, int sec) {
    (void)ch; (void)sec;
    return ESP_OK;
}

static inline esp_err_t esp_wifi_set_promiscuous(bool en) {
    (void)en;
    return ESP_OK;
}
#endif
