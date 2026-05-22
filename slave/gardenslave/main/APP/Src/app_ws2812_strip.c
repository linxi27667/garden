#include "app_ws2812_strip.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"

static const char* TAG = "WS2812_STRIP";

static led_strip_handle_t s_strip = NULL;
static uint16_t s_led_count = WS2812_STRIP_LED_COUNT;

static uint8_t Scale_Channel(uint8_t value, uint8_t brightness) {
    if (brightness > 100) {
        brightness = 100;
    }
    return (uint8_t)(((uint16_t)value * brightness) / 100);
}

static void Set_All(uint8_t red, uint8_t green, uint8_t blue) {
    if (s_strip == NULL) {
        return;
    }

    for (uint16_t i = 0; i < s_led_count; i++) {
        led_strip_set_pixel(s_strip, i, red, green, blue);
    }
    led_strip_refresh(s_strip);
}

static void Wheel(uint8_t pos, uint8_t brightness, uint8_t* red, uint8_t* green, uint8_t* blue) {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    if (pos < 85) {
        r = (uint8_t)(255 - pos * 3);
        g = (uint8_t)(pos * 3);
    } else if (pos < 170) {
        pos -= 85;
        g = (uint8_t)(255 - pos * 3);
        b = (uint8_t)(pos * 3);
    } else {
        pos -= 170;
        b = (uint8_t)(255 - pos * 3);
        r = (uint8_t)(pos * 3);
    }

    *red = Scale_Channel(r, brightness);
    *green = Scale_Channel(g, brightness);
    *blue = Scale_Channel(b, brightness);
}

bool App_WS2812_Strip_Init(gpio_num_t gpio, uint16_t led_count) {
    if (s_strip != NULL) {
        return true;
    }

    if (led_count == 0) {
        led_count = WS2812_STRIP_LED_COUNT;
    }
    s_led_count = led_count;

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio,
        .max_leds = s_led_count,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = WS2812_STRIP_RMT_RES_HZ,
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false,
        }
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init WS2812B strip on GPIO%d: %s", gpio, esp_err_to_name(err));
        s_strip = NULL;
        return false;
    }

    led_strip_clear(s_strip);
    ESP_LOGI(TAG, "WS2812B strip initialized: GPIO%d, leds=%d", gpio, s_led_count);
    return true;
}

void App_WS2812_Strip_Apply(bool enabled,
                            uint8_t red,
                            uint8_t green,
                            uint8_t blue,
                            uint8_t brightness,
                            uint8_t effect) {
    if (s_strip == NULL) {
        return;
    }

    if (!enabled || brightness == 0) {
        led_strip_clear(s_strip);
        return;
    }

    if (brightness > 100) {
        brightness = 100;
    }

    uint32_t tick = xTaskGetTickCount();

    switch (effect) {
    case IOT_STRIP_EFFECT_BREATHE: {
        uint32_t phase = (tick / pdMS_TO_TICKS(40)) % 100;
        uint8_t wave = (phase < 50) ? (uint8_t)(phase * 2) : (uint8_t)((100 - phase) * 2);
        uint8_t local_brightness = (uint8_t)((brightness * (20 + wave * 80 / 100)) / 100);
        Set_All(Scale_Channel(red, local_brightness),
                Scale_Channel(green, local_brightness),
                Scale_Channel(blue, local_brightness));
        break;
    }
    case IOT_STRIP_EFFECT_RAINBOW: {
        uint8_t offset = (uint8_t)((tick / pdMS_TO_TICKS(60)) & 0xFF);
        for (uint16_t i = 0; i < s_led_count; i++) {
            uint8_t r;
            uint8_t g;
            uint8_t b;
            Wheel((uint8_t)(offset + (i * 256 / s_led_count)), brightness, &r, &g, &b);
            led_strip_set_pixel(s_strip, i, r, g, b);
        }
        led_strip_refresh(s_strip);
        break;
    }
    case IOT_STRIP_EFFECT_ALERT: {
        bool on = ((tick / pdMS_TO_TICKS(250)) % 2) == 0;
        if (on) {
            Set_All(Scale_Channel(red, brightness),
                    Scale_Channel(green, brightness),
                    Scale_Channel(blue, brightness));
        } else {
            led_strip_clear(s_strip);
        }
        break;
    }
    case IOT_STRIP_EFFECT_SOLID:
    default:
        Set_All(Scale_Channel(red, brightness),
                Scale_Channel(green, brightness),
                Scale_Channel(blue, brightness));
        break;
    }
}
