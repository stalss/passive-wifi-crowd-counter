/* Mock netif */
#ifndef MOCK_ESP_NETIF_H
#define MOCK_ESP_NETIF_H
typedef void *esp_netif_t;
typedef enum { ESP_OK = 0 } esp_err_t;
static inline esp_err_t esp_netif_init(void) { return ESP_OK; }
static inline esp_err_t esp_event_loop_create_default(void) { return ESP_OK; }
static inline esp_netif_t *esp_netif_create_default_wifi_sta(void) { return (void*)0; }
#endif
