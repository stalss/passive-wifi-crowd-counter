/* Mock NVS */
#ifndef MOCK_NVS_H
#define MOCK_NVS_H
typedef enum { ESP_OK = 0, ESP_ERR_NVS_NO_FREE_PAGES = 0x103, ESP_ERR_NVS_NEW_VERSION_FOUND = 0x104 } esp_err_t;
typedef void *nvs_handle_t;
typedef enum { NVS_READWRITE = 0 } nvs_open_mode_t;
static inline esp_err_t nvs_flash_init(void) { return ESP_OK; }
static inline esp_err_t nvs_flash_erase(void) { return ESP_OK; }
static inline esp_err_t nvs_open(const char *ns, nvs_open_mode_t m, nvs_handle_t *h) { (void)ns;(void)m;(void)h; return ESP_OK; }
static inline esp_err_t nvs_close(nvs_handle_t h) { (void)h; return ESP_OK; }
#endif
