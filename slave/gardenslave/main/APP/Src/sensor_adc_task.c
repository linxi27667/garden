#include "sensor_adc_task.h"
#include "iot_control_task.h"
#include "sensor_config.h"
#include "mqtt_receive.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
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
    adc1_config_channel_atten(LIGHT_ADC_CHANNEL, LIGHT_ADC_ATTEN);
    ESP_LOGI(TAG, "ADC initialized: Soil=CH0(GPIO1), Light=CH2(GPIO3)");
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
    ets_delay_us(18000);
    gpio_set_level((gpio_num_t)pin, 1);
    ets_delay_us(30);

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
        ets_delay_us(1);
        if (++timeout > 80) return false;
    }
    timeout = 0;
    while (gpio_get_level((gpio_num_t)pin) == 0) {
        ets_delay_us(1);
        if (++timeout > 80) return false;
    }
    timeout = 0;
    while (gpio_get_level((gpio_num_t)pin) == 1) {
        ets_delay_us(1);
        if (++timeout > 80) return false;
    }

    for (int i = 0; i < 40; i++) {
        timeout = 0;
        while (gpio_get_level((gpio_num_t)pin) == 0) {
            ets_delay_us(1);
            if (++timeout > 100) return false;
        }
        int high_us = 0;
        while (gpio_get_level((gpio_num_t)pin) == 1) {
            ets_delay_us(1);
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

static uint8_t g_rain_exceed_count = 0;
static float g_temperature = 0;
static float g_humidity = 0;
static bool g_dht_valid = false;

static void Sensor_ADC_Task(void* arg) {
    ESP_LOGI(TAG, "Sensor ADC task started");
    HW_ADC_Init();

    while (1) {
        int32_t soil_mv = ADC_Get_Voltage_MV(SOIL_ADC_CHANNEL);
        int32_t light_mv = ADC_Get_Voltage_MV(LIGHT_ADC_CHANNEL);

        int32_t soil_percent = (soil_mv * 100) / 3100;
        if (soil_percent > 100) soil_percent = 100;
        if (soil_percent < 0) soil_percent = 0;

        int32_t light_percent = (light_mv * 100) / 3100;
        if (light_percent > 100) light_percent = 100;
        if (light_percent < 0) light_percent = 0;

        uint8_t rain_wet = (light_mv <= RAIN_THRESHOLD_LIGHT) ? 1 : 0;

        if (rain_wet) {
            if (++g_rain_exceed_count >= RAIN_TRIGGER_COUNT) {
                if (g_device_flags.auto_mode) {
                    DEVICE_FLAGS_LOCK();
                    if (g_device_flags.servo[0] != SERVO_MIN_ANGLE) {
                        ESP_LOGI(TAG, "[RAIN] Auto collecting clothes");
                        g_device_flags.servo[0] = SERVO_MIN_ANGLE;
                        g_rain_alert_triggered = true;
                    }
                    DEVICE_FLAGS_UNLOCK();
                }
            }
        } else {
            g_rain_exceed_count = 0;
        }

        g_dht_valid = DHT11_Read(DHT_PIN, &g_temperature, &g_humidity);

        if (g_device_flags.auto_mode && g_dht_valid) {
            DEVICE_FLAGS_LOCK();
            if (g_temperature > -40.0f && g_temperature < 80.0f) {
                if (g_temperature > 35.0f) {
                    if (g_device_flags.relay[0] == OFF) g_device_flags.relay[0] = ON;
                    if (g_device_flags.relay[2] == OFF) g_device_flags.relay[2] = ON;
                } else if (g_temperature < 10.0f) {
                    if (g_device_flags.relay[0] == ON) g_device_flags.relay[0] = OFF;
                    if (g_device_flags.relay[2] == ON && g_humidity > 40.0f) g_device_flags.relay[2] = OFF;
                }
            }
            if (g_humidity > 0.0f && g_humidity < 100.0f) {
                if (g_humidity < 20.0f) {
                    if (g_device_flags.relay[2] == OFF) g_device_flags.relay[2] = ON;
                } else if (g_humidity > 80.0f) {
                    if (g_device_flags.relay[0] == OFF) g_device_flags.relay[0] = ON;
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
            if (client && mac) {
                char topic[64];
                snprintf(topic, sizeof(topic), MQTT_TOPIC_SENSOR_PREFIX "%s", mac);

                char payload[256];
                int len = snprintf(payload, sizeof(payload),
                    "{\"soil_mv\":%d,\"light_mv\":%d,\"fire_status\":%d,"
                    "\"rain_status\":%d,\"help_status\":%d,"
                    "\"temperature\":%.1f,\"humidity\":%.1f}",
                    (int)soil_mv, (int)light_mv,
                    (int)g_device_flags.fire_status,
                    rain_wet, g_help_detected ? 1 : 0,
                    g_temperature, g_humidity);

                if (len > 0 && len < (int)sizeof(payload)) {
                    esp_mqtt_client_publish(client, topic, payload, len, 0, 0);
                }
            }
            ESP_LOGI(TAG, "Soil:%dmV(%d%%) Light:%dmV(%d%%) Rain:%s Fire:%d Help:%d Temp:%.1f Hum:%.1f",
                     (int)soil_mv, (int)soil_percent, (int)light_mv, (int)light_percent,
                     rain_wet ? "WET" : "DRY", fire_status, help_status,
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
