#include "sensor_adc_task.h"
#include "iot_control_task.h"
#include "sensor_config.h"
#include "mqtt_receive.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "SENSOR_ADC";

static void HW_ADC_Init(void) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_ADC_CHANNEL, SOIL_ADC_ATTEN);
    adc1_config_channel_atten(RAIN_ADC_CHANNEL, RAIN_ADC_ATTEN);
    ESP_LOGI(TAG, "ADC initialized: Soil=GPIO1(CH0), Rain=GPIO4(CH3)");
}

static int32_t ADC_Get_Voltage_MV(adc1_channel_t channel) {
    int sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += adc1_get_raw(channel);
    }
    return (sum * 3100) / (4095 * 16);
}

static bool DHT11_Read(uint8_t pin, float* temperature, float* humidity) {
    uint8_t data[5] = {0};

    gpio_config_t out_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&out_conf);
    gpio_set_level((gpio_num_t)pin, 0);
    esp_rom_delay_us(18000);
    gpio_set_level((gpio_num_t)pin, 1);
    esp_rom_delay_us(30);

    gpio_config_t in_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&in_conf);

    int timeout = 0;
    while (gpio_get_level((gpio_num_t)pin) == 1) {
        esp_rom_delay_us(1);
        if (++timeout > 80) return false;
    }
    timeout = 0;
    while (gpio_get_level((gpio_num_t)pin) == 0) {
        esp_rom_delay_us(1);
        if (++timeout > 80) return false;
    }
    timeout = 0;
    while (gpio_get_level((gpio_num_t)pin) == 1) {
        esp_rom_delay_us(1);
        if (++timeout > 80) return false;
    }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level((gpio_num_t)pin) == 0) {
            esp_rom_delay_us(1);
            if (++timeout > 100) return false;
        }
        int high_us = 0;
        while (gpio_get_level((gpio_num_t)pin) == 1) {
            esp_rom_delay_us(1);
            if (++high_us > 100) return false;
        }
        data[i / 8] <<= 1;
        if (high_us > 40) {
            data[i / 8] |= 1;
        }
    }

    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return false;
    }

    if (humidity) *humidity = (float)data[0] + (float)data[1] * 0.1f;
    if (temperature) *temperature = (float)data[2] + (float)data[3] * 0.1f;
    return true;
}

static uint16_t g_rain_exceed_count = 0;
static float g_temperature = 0;
static float g_humidity = 0;
static bool g_dht_valid = false;

#define AUTO_CTRL_COOLDOWN_MS  30000
static uint32_t g_relay_manual_time[RELAY_COUNT] = {0};
static bool g_watering_active = false;
static uint32_t g_watering_started_ms = 0;
static uint32_t g_watering_cooldown_until_ms = 0;

void Sensor_ADC_Notify_Relay_Manual(uint8_t relay_index) {
    if (relay_index < RELAY_COUNT) {
        g_relay_manual_time[relay_index] = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (relay_index == 0) {
            g_device_flags.auto_fan = false;
        } else if (relay_index == 1) {
            g_device_flags.auto_watering = false;
            g_watering_active = false;
        } else if (relay_index == 2) {
            g_device_flags.auto_atomizer = false;
        }
    }
}

void Sensor_ADC_Notify_Servo_Manual(uint8_t servo_index) {
    if (servo_index < SERVO_COUNT) {
        g_device_flags.auto_rain_shelter = false;
    }
}

static void Sensor_ADC_Task(void* arg) {
    ESP_LOGI(TAG, "Sensor ADC task started");
    HW_ADC_Init();

    while (1) {
        int32_t soil_mv = ADC_Get_Voltage_MV(SOIL_ADC_CHANNEL);
        int32_t rain_mv = ADC_Get_Voltage_MV(RAIN_ADC_CHANNEL);

        int32_t soil_percent = (soil_mv * 100) / 3100;
        if (soil_percent > 100) soil_percent = 100;
        if (soil_percent < 0) soil_percent = 0;

        int32_t rain_percent = 100 - (rain_mv * 100) / 3100;
        if (rain_percent > 100) rain_percent = 100;
        if (rain_percent < 0) rain_percent = 0;

        uint8_t rain_wet = (rain_percent > RAIN_THRESHOLD_PERCENT) ? 1 : 0;

        if (rain_wet) {
            if (g_rain_exceed_count < RAIN_TRIGGER_COUNT + 10) {
                g_rain_exceed_count++;
            }
            if (g_rain_exceed_count >= RAIN_TRIGGER_COUNT) {
                if (g_device_flags.auto_mode && g_device_flags.auto_rain_shelter) {
                    DEVICE_FLAGS_LOCK();
                    if (g_device_flags.servo[0] != SERVO_MAX_ANGLE) {
                        ESP_LOGI(TAG, "[RAIN] Auto deploying rain shelter");
                        g_device_flags.servo[0] = SERVO_MAX_ANGLE;
                        g_device_flags.light[1] = SWITCH_ON;
                        g_device_flags.strip_red = 70;
                        g_device_flags.strip_green = 120;
                        g_device_flags.strip_blue = 255;
                        g_device_flags.strip_brightness = 32;
                        g_device_flags.strip_effect = IOT_STRIP_EFFECT_BREATHE;
                        g_device_flags.ambience_scene = IOT_AMBIENCE_SCENE_RAIN;
                        xTaskNotify(s_iot_ctrl_task_handle, NOTIFY_RAIN_BIT, eSetBits);
                    }
                    DEVICE_FLAGS_UNLOCK();
                }
            }
        } else {
            g_rain_exceed_count = 0;
        }

        g_dht_valid = DHT11_Read(DHT_PIN, &g_temperature, &g_humidity);

        if (g_device_flags.auto_mode && g_device_flags.auto_watering && g_device_flags.fire_status != FIRE_CONFIRMED) {
            uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            DEVICE_FLAGS_LOCK();
            if (rain_wet) {
                if (g_device_flags.relay[1] == SWITCH_ON) {
                    g_device_flags.relay[1] = SWITCH_OFF;
                }
                g_watering_active = false;
                g_watering_cooldown_until_ms = now_ms + WATER_PUMP_COOLDOWN_MS;
            } else if (g_watering_active) {
                bool wet_enough = soil_percent >= SOIL_WATER_STOP_PERCENT;
                bool timed_out = (now_ms - g_watering_started_ms) >= WATER_PUMP_MAX_RUN_MS;
                if (wet_enough || timed_out) {
                    g_device_flags.relay[1] = SWITCH_OFF;
                    g_watering_active = false;
                    g_watering_cooldown_until_ms = now_ms + WATER_PUMP_COOLDOWN_MS;
                    ESP_LOGI(TAG, "[SOIL] Auto watering stopped: soil=%d%% timeout=%d",
                             (int)soil_percent, timed_out ? 1 : 0);
                }
            } else if (soil_percent < SOIL_WATER_START_PERCENT &&
                       now_ms >= g_watering_cooldown_until_ms) {
                g_device_flags.relay[1] = SWITCH_ON;
                g_watering_active = true;
                g_watering_started_ms = now_ms;
                ESP_LOGI(TAG, "[SOIL] Auto watering started: soil=%d%%", (int)soil_percent);
            }
            DEVICE_FLAGS_UNLOCK();
        }

        if (g_device_flags.auto_mode && g_dht_valid) {
            uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            DEVICE_FLAGS_LOCK();
            if (g_temperature > -40.0f && g_temperature < 80.0f) {
                if (g_temperature >= TEMP_COOLING_ON_C) {
                    if (g_device_flags.auto_fan &&
                        (now_ms - g_relay_manual_time[0]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[0] == SWITCH_OFF) g_device_flags.relay[0] = SWITCH_ON;
                    }
                    if (g_device_flags.auto_atomizer &&
                        (now_ms - g_relay_manual_time[2]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[2] == SWITCH_OFF) g_device_flags.relay[2] = SWITCH_ON;
                    }
                } else if (g_temperature <= TEMP_COOLING_OFF_C) {
                    if (g_device_flags.auto_fan &&
                        (now_ms - g_relay_manual_time[0]) >= AUTO_CTRL_COOLDOWN_MS &&
                        g_humidity <= HUMIDITY_FAN_OFF_PERCENT) {
                        if (g_device_flags.relay[0] == SWITCH_ON) g_device_flags.relay[0] = SWITCH_OFF;
                    }
                    if (g_device_flags.auto_atomizer &&
                        (now_ms - g_relay_manual_time[2]) >= AUTO_CTRL_COOLDOWN_MS &&
                        g_humidity >= HUMIDITY_DRY_OFF_PERCENT) {
                        if (g_device_flags.relay[2] == SWITCH_ON) g_device_flags.relay[2] = SWITCH_OFF;
                    }
                }
            }
            if (g_humidity > 0.0f && g_humidity < 100.0f) {
                if (g_humidity <= HUMIDITY_DRY_ON_PERCENT) {
                    if (g_device_flags.auto_atomizer &&
                        (now_ms - g_relay_manual_time[2]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[2] == SWITCH_OFF) g_device_flags.relay[2] = SWITCH_ON;
                    }
                } else if (g_humidity >= HUMIDITY_DRY_OFF_PERCENT &&
                           g_temperature < TEMP_COOLING_ON_C) {
                    if (g_device_flags.auto_atomizer &&
                        (now_ms - g_relay_manual_time[2]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[2] == SWITCH_ON) g_device_flags.relay[2] = SWITCH_OFF;
                    }
                }

                if (g_humidity >= HUMIDITY_FAN_ON_PERCENT) {
                    if (g_device_flags.auto_fan &&
                        (now_ms - g_relay_manual_time[0]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[0] == SWITCH_OFF) g_device_flags.relay[0] = SWITCH_ON;
                    }
                } else if (g_humidity <= HUMIDITY_FAN_OFF_PERCENT &&
                           g_temperature < TEMP_COOLING_ON_C) {
                    if (g_device_flags.auto_fan &&
                        (now_ms - g_relay_manual_time[0]) >= AUTO_CTRL_COOLDOWN_MS) {
                        if (g_device_flags.relay[0] == SWITCH_ON) g_device_flags.relay[0] = SWITCH_OFF;
                    }
                }
            }
            DEVICE_FLAGS_UNLOCK();
        }

        uint8_t fire_status = (uint8_t)g_device_flags.fire_status;
        uint8_t help_status = g_help_detected ? 1 : 0;

        static uint32_t last_report = 0;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_report >= 5000) {
            esp_mqtt_client_handle_t client = MQTT_Get_Client();
            const char* mac = MQTT_Get_MAC_Str();
            if (client && mac && MQTT_Is_Connected()) {
                char topic[64];
                snprintf(topic, sizeof(topic), MQTT_TOPIC_SENSOR_PREFIX "%s", mac);

                char payload[384];
                int len = snprintf(payload, sizeof(payload),
                    "{\"soil_mv\":%d,\"soil_percent\":%d,\"rain_mv\":%d,\"rain_percent\":%d,\"fire_status\":%d,"
                    "\"rain_status\":%d,\"help_status\":%d,"
                    "\"temperature\":%.1f,\"humidity\":%.1f}",
                    (int)soil_mv, (int)soil_percent,
                    (int)rain_mv, (int)rain_percent,
                    (int)g_device_flags.fire_status,
                    rain_wet, g_help_detected ? 1 : 0,
                    g_temperature, g_humidity);

                if (len > 0 && len < (int)sizeof(payload)) {
                    esp_mqtt_client_publish(client, topic, payload, len, 0, 0);
                }
            } else {
                static uint32_t skip_count = 0;
                if ((skip_count++ % 6) == 0) {
                    ESP_LOGW(TAG, "MQTT not connected, sensor report skipped");
                }
            }
            ESP_LOGD(TAG, "Soil:%dmV(%d%%) Rain:%dmV(%d%%) Fire:%d Help:%d Temp:%.1f Hum:%.1f",
                     (int)soil_mv, (int)soil_percent,
                     (int)rain_mv, (int)rain_percent, fire_status, help_status,
                     g_temperature, g_humidity);
            last_report = now;
        }

        vTaskDelay(pdMS_TO_TICKS(ADC_SAMPLE_INTERVAL_MS));
    }
}

void Sensor_ADC_Task_Init(void) {
    BaseType_t ret = xTaskCreatePinnedToCore(
        Sensor_ADC_Task,
        "Sensor_ADC",
        4096,
        NULL,
        4,
        NULL,
        1
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor ADC task");
    } else {
        ESP_LOGI(TAG, "Sensor ADC task initialized");
    }
}
