#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_mac.h"

#include "app_wifi.h"
#include "mqtt_receive.h"
#include "mqtt_heartbeat.h"
#include "iot_control_task.h"
#include "sensor_adc_task.h"

static const char* TAG = "MAIN";

void app_main(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   Garden Slave Device Starting...");
    ESP_LOGI(TAG, "========================================");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Connecting to WiFi...");
    App_WiFi_System_Init();

    ESP_LOGI(TAG, "Initializing MQTT...");
    MQTT_Receive_Init();

    ESP_LOGI(TAG, "Initializing Device Control...");
    Iot_Control_Task_Init();

    ESP_LOGI(TAG, "Initializing Heartbeat...");
    MQTT_Heartbeat_Init();

    ESP_LOGI(TAG, "Initializing Sensors...");
    Sensor_ADC_Task_Init();

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "   Garden Slave Ready!");
    ESP_LOGI(TAG, "========================================");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
