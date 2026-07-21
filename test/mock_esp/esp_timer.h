/* Mock ESP-IDF headers for native Linux testing */
#ifndef MOCK_ESP_TIMER_H
#define MOCK_ESP_TIMER_H
#include <stdint.h>
#include <time.h>

static inline int64_t esp_timer_get_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}
#endif
