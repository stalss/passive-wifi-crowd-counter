/* Mock GPIO */
#ifndef MOCK_GPIO_H
#define MOCK_GPIO_H
#include <stdint.h>

typedef enum { GPIO_NUM_0 = 0 } gpio_num_t;

static inline int gpio_set_level(gpio_num_t pin, uint32_t val) {
    (void)pin; (void)val;
    return 0;
}
#endif
