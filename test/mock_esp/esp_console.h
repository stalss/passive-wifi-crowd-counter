/* Mock esp_console */
#ifndef MOCK_ESP_CONSOLE_H
#define MOCK_ESP_CONSOLE_H
typedef void *esp_console_dev_uart_config_t;
typedef void *esp_console_repl_t;
typedef enum { ESP_OK = 0 } esp_err_t;
static inline esp_err_t esp_console_new_repl_uart(void *cfg, void *repl_config, esp_console_repl_t *repl) {
    (void)cfg;(void)repl_config;(void)repl;
    return ESP_OK;
}
#endif
