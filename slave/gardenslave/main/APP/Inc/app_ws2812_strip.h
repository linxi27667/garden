#ifndef APP_WS2812_STRIP_H
#define APP_WS2812_STRIP_H

#include <stdint.h>
#include <stdbool.h>
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WS2812_STRIP_LED_COUNT       30
#define WS2812_STRIP_RMT_RES_HZ      (10 * 1000 * 1000)

bool App_WS2812_Strip_Init(gpio_num_t gpio, uint16_t led_count);
void App_WS2812_Strip_Apply(bool enabled,
                            uint8_t red,
                            uint8_t green,
                            uint8_t blue,
                            uint8_t brightness,
                            uint8_t effect);

#ifdef __cplusplus
}
#endif

#endif
