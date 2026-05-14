#include "iot_controller.h"
#include "bsp_iot_controller.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cJSON.h>

static const char* TAG = "APP_IOT";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;

static iot_controller_t g_iot_controller;

#define MAX_MQTT_CMD_CALLBACKS 4
static struct {
    char topic_prefix[64];
    iot_mqtt_cmd_callback_t callback;
} s_mqtt_cmd_callbacks[MAX_MQTT_CMD_CALLBACKS];
static int s_mqtt_cmd_callback_count = 0;

static void HW_MQTT_Recv_Callback(const char* topic, int topic_len, const char* data, int data_len);
static bool HW_MQTT_Init(void);
static bool HW_MQTT_Publish(const char* topic, const uint8_t* data, uint16_t len);

#define LOCAL_GPIO_1_PIN    GPIO_NUM_48
#define LOCAL_GPIO_2_PIN    GPIO_NUM_46
#define LOCAL_GPIO_3_PIN    GPIO_NUM_14
#define LOCAL_GPIO_4_PIN    GPIO_NUM_38

static void HW_Gpio_Init(void* port, uint16_t pin, iot_gpio_mode_t mode, uint8_t default_level) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << pin),
        .mode = mode == IOT_GPIO_MODE_OUTPUT ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = mode == IOT_GPIO_MODE_INPUT ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    if (mode == IOT_GPIO_MODE_OUTPUT) {
        gpio_set_level((gpio_num_t)pin, default_level);
    }
}

static void HW_Gpio_Write(void* port, uint16_t pin, uint8_t level) {
    gpio_set_level((gpio_num_t)pin, level);
}

static int HW_Gpio_Read(void* port, uint16_t pin) {
    return gpio_get_level((gpio_num_t)pin);
}

static bool Dispatch_MQTT_Callbacks(const char* topic, const char* data, int data_len) {
    bool dispatched = false;

    if (topic == NULL) {
        return false;
    }

    for (int i = 0; i < s_mqtt_cmd_callback_count; i++) {
        if (s_mqtt_cmd_callbacks[i].callback != NULL &&
            strstr(topic, s_mqtt_cmd_callbacks[i].topic_prefix) != NULL) {
            s_mqtt_cmd_callbacks[i].callback(topic, data, data_len);
            dispatched = true;
        }
    }

    return dispatched;
}

static void HW_MQTT_Recv_Callback(const char* topic, int topic_len, const char* data, int data_len) {
    ESP_LOGD(TAG, "MQTT received %d bytes on topic: %.*s", data_len, topic_len, topic);

    char topic_buf[128] = {0};
    if (topic_len > 0 && topic_len < (int)sizeof(topic_buf)) {
        memcpy(topic_buf, topic, topic_len);
        topic_buf[topic_len] = '\0';
    }

    bool callback_dispatched = Dispatch_MQTT_Callbacks(topic_buf, data, data_len);
    if (strstr(topic_buf, "xiaozhi/garden/") != NULL && callback_dispatched) {
        return;
    }

    uint8_t src_mac[6] = {0};
    const char* mac_str = NULL;
    
    if (strstr(topic_buf, MQTT_TOPIC_HEARTBEAT_PREFIX) != NULL) {
        mac_str = topic_buf + strlen(MQTT_TOPIC_HEARTBEAT_PREFIX);
    } else if (strstr(topic_buf, MQTT_TOPIC_RESP_PREFIX) != NULL) {
        mac_str = topic_buf + strlen(MQTT_TOPIC_RESP_PREFIX);
    } else if (strstr(topic_buf, MQTT_TOPIC_ANNOUNCE_PREFIX) != NULL) {
        mac_str = topic_buf + strlen(MQTT_TOPIC_ANNOUNCE_PREFIX);
    } else if (strstr(topic_buf, MQTT_TOPIC_SENSOR_PREFIX) != NULL) {
        mac_str = topic_buf + strlen(MQTT_TOPIC_SENSOR_PREFIX);
    } else if (strstr(topic_buf, MQTT_TOPIC_EMERGENCY_PREFIX) != NULL) {
        mac_str = topic_buf + strlen(MQTT_TOPIC_EMERGENCY_PREFIX);
    }
    
    if (mac_str != NULL && strlen(mac_str) == 12) {
        for (int i = 0; i < 6; i++) {
            char hex[3] = {mac_str[i*2], mac_str[i*2+1], '\0'};
            src_mac[i] = (uint8_t)strtol(hex, NULL, 16);
        }
        ESP_LOGD(TAG, "Parsed MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]);
    }
    
    if (data_len >= (int)sizeof(iot_command_packet_t)) {
        iot_command_packet_t* cmd = (iot_command_packet_t*)data;
        if (cmd->command == IOT_CMD_HEARTBEAT) {
            IOT_Update_Heartbeat(&g_iot_controller, src_mac);
            return;
        }
        if (cmd->command == IOT_CMD_SENSOR_REPORT) {
            IOT_Process_Sensor_Data(&g_iot_controller, src_mac, (const uint8_t*)data, data_len);
            return;
        }
    }

    if (strstr(topic_buf, MQTT_TOPIC_SENSOR_PREFIX) != NULL && data_len > 0 && data[0] == '{') {
        IOT_Process_Sensor_Data(&g_iot_controller, src_mac, (const uint8_t*)data, data_len);
        return;
    }

    if (strstr(topic_buf, MQTT_TOPIC_EMERGENCY_PREFIX) != NULL && data_len > 0) {
        cJSON* root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON* type_item = cJSON_GetObjectItem(root, "emergency_type");
            cJSON* detail_item = cJSON_GetObjectItem(root, "detail");
            int emergency_type = type_item ? type_item->valueint : 0;
            const char* detail = detail_item ? detail_item->valuestring : "unknown";

            const char* type_str = "unknown";
            if (emergency_type == 0) type_str = "fire";
            else if (emergency_type == 1) type_str = "help";
            else if (emergency_type == 2) type_str = "rain";

            ESP_LOGW(TAG, "EMERGENCY received from slave: type=%s, detail=%s", type_str, detail);

            IOT_Update_Heartbeat(&g_iot_controller, src_mac);

            App_IOT_Emergency_Alert(type_str, detail);

            cJSON_Delete(root);
        }
        return;
    }

    IOT_Process_Remote_Response(&g_iot_controller, src_mac, (const uint8_t*)data, data_len);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected, subscribing...");
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/iot/resp/+", 0);
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/iot/heartbeat/+", 0);
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/iot/announce/+", 1);
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/iot/sensor/+", 0);
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/iot/emergency/+", 1);
            esp_mqtt_client_subscribe(s_mqtt_client, "xiaozhi/garden/cmd", 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        case MQTT_EVENT_DATA:
            HW_MQTT_Recv_Callback(event->topic, event->topic_len, event->data, event->data_len);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;
        default:
            break;
    }
}

static bool HW_MQTT_Init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://8.134.167.240",  // 自己的服务器
        .broker.address.port = 1883,
        .credentials.client_id = "xiaozhi_master_esp32",   // 唯一 ID 避免冲突
        .session.keepalive = 30,
        .network.disable_auto_reconnect = false,           // 启用自动重连
        .network.reconnect_timeout_ms = 5000,              // 5秒重连
    };
    
    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT init failed");
        return false;
    }
    
    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t ret = esp_mqtt_client_start(s_mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "MQTT start failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "MQTT initialized");
    return true;
}

static bool HW_MQTT_Publish(const char* topic, const uint8_t* data, uint16_t len) {
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return false;
    }
    
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, (const char*)data, len, 1, 0);
    return msg_id >= 0;
}

static void HW_Delay_Ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void HW_Log_Info(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_INFO, tag, fmt, args);
    va_end(args);
}

static void HW_Log_Error(const char* tag, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    esp_log_writev(ESP_LOG_ERROR, tag, fmt, args);
    va_end(args);
}

#define DEVICE_TIMEOUT_MS   60000

static void Device_Timeout_Task(void* arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        IOT_Check_Device_Timeout(&g_iot_controller, DEVICE_TIMEOUT_MS);
    }
}

void App_IOT_System_Init(void) {
    // 设置日志级别为DEBUG，INFO日志将显示为白色（无ANSI颜色）
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_log_level_set("BSP_IOT", ESP_LOG_DEBUG);
    
    ESP_LOGI(TAG, "Initializing IOT Controller...");

    g_iot_controller.local_gpio_count = 0;
    g_iot_controller.remote_device_count = 0;
    g_iot_controller.Gpio_Init = HW_Gpio_Init;
    g_iot_controller.Gpio_Write = HW_Gpio_Write;
    g_iot_controller.Gpio_Read = HW_Gpio_Read;
    g_iot_controller.Mqtt_Init = HW_MQTT_Init;
    g_iot_controller.Mqtt_Publish = HW_MQTT_Publish;
    g_iot_controller.Delay_Ms = HW_Delay_Ms;
    g_iot_controller.Log_Info = HW_Log_Info;
    g_iot_controller.Log_Error = HW_Log_Error;

    if (g_iot_controller.Mqtt_Init) {
        g_iot_controller.Mqtt_Init();
    }

    xTaskCreate(Device_Timeout_Task, "iot_timeout", 2048, NULL, 1, NULL);

    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Auto-discovering nearby devices...");
    App_IOT_Discover_Devices();
}

bool App_IOT_Set_GPIO(const char* device_name, const char* gpio_name, uint8_t level) {
    if (device_name == NULL || strcmp(device_name, "local") == 0) {
        for (int i = 0; i < g_iot_controller.local_gpio_count; i++) {
            if (strcmp(g_iot_controller.local_gpios[i].name, gpio_name) == 0) {
                IOT_Set_Local_GPIO(&g_iot_controller, i, level);
                return true;
            }
        }
        ESP_LOGW(TAG, "Local GPIO '%s' not found", gpio_name);
        return false;
    }
    
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Remote device '%s' not found", device_name);
        return false;
    }
    
    for (int i = 0; i < g_iot_controller.remote_devices[device_index].gpio_count; i++) {
        if (strcmp(g_iot_controller.remote_devices[device_index].gpios[i].name, gpio_name) == 0) {
            return IOT_Set_Remote_GPIO(&g_iot_controller, device_index, i, level);
        }
    }
    
    ESP_LOGW(TAG, "GPIO '%s' not found on device '%s'", gpio_name, device_name);
    return false;
}

int App_IOT_Get_GPIO(const char* device_name, const char* gpio_name) {
    if (device_name == NULL || strcmp(device_name, "local") == 0) {
        for (int i = 0; i < g_iot_controller.local_gpio_count; i++) {
            if (strcmp(g_iot_controller.local_gpios[i].name, gpio_name) == 0) {
                return IOT_Get_Local_GPIO(&g_iot_controller, i);
            }
        }
    }
    return -1;
}

int App_IOT_Get_Device_Count(void) {
    return g_iot_controller.remote_device_count;
}

int App_IOT_Get_Local_GPIO_Count(void) {
    return g_iot_controller.local_gpio_count;
}

const char* App_IOT_Get_Device_Name(int index) {
    if (index < 0 || index >= g_iot_controller.remote_device_count) {
        return NULL;
    }
    return g_iot_controller.remote_devices[index].device_name;
}

const char* App_IOT_Get_Local_GPIO_Name(int index) {
    if (index < 0 || index >= g_iot_controller.local_gpio_count) {
        return NULL;
    }
    return g_iot_controller.local_gpios[index].name;
}

bool App_IOT_Add_Remote_Device(const char* name, const uint8_t* mac, int gpio_count) {
    int index = IOT_Add_Remote_Device(&g_iot_controller, name, mac, gpio_count);
    return index >= 0;
}

void App_IOT_Discover_Devices(void) {
    IOT_Discover_Devices(&g_iot_controller);
}

bool App_IOT_Set_Servo(const char* device_name, const char* gpio_name, uint8_t angle) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Remote device '%s' not found", device_name);
        return false;
    }
    
    for (int i = 0; i < g_iot_controller.remote_devices[device_index].gpio_count; i++) {
        if (strcmp(g_iot_controller.remote_devices[device_index].gpios[i].name, gpio_name) == 0) {
            return IOT_Set_Remote_Servo(&g_iot_controller, device_index, i, angle);
        }
    }
    
    ESP_LOGW(TAG, "GPIO '%s' not found on device '%s'", gpio_name, device_name);
    return false;
}

bool App_IOT_Set_Light(const char* device_name, uint8_t light_index, bool on) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Remote device '%s' not found", device_name);
        return false;
    }
    return IOT_Set_Light(&g_iot_controller, device_index, light_index, on ? 1 : 0);
}

bool App_IOT_Set_Relay(const char* device_name, uint8_t relay_index, bool on) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Remote device '%s' not found", device_name);
        return false;
    }
    return IOT_Set_Relay(&g_iot_controller, device_index, relay_index, on ? 1 : 0);
}

bool App_IOT_Set_Servo_ByIndex(const char* device_name, uint8_t servo_index, uint8_t angle) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Remote device '%s' not found", device_name);
        return false;
    }
    return IOT_Set_Servo(&g_iot_controller, device_index, servo_index, angle);
}

bool App_IOT_Broadcast_All_Off(void) {
    return IOT_Broadcast_All_Off(&g_iot_controller);
}

bool App_IOT_Broadcast_All_On(void) {
    return IOT_Broadcast_All_On(&g_iot_controller);
}

bool App_IOT_Broadcast_Lights_Off(void) {
    return IOT_Broadcast_Lights_Off(&g_iot_controller);
}

bool App_IOT_Broadcast_Lights_On(void) {
    return IOT_Broadcast_Lights_On(&g_iot_controller);
}

void App_IOT_Get_All_Status(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;

    int offset = 0;

    offset += snprintf(buffer + offset, buffer_size - offset,
                       "{\"local_gpios\":[");

    for (int i = 0; i < g_iot_controller.local_gpio_count && offset < buffer_size - 100; i++) {
        if (i > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "{\"name\":\"%s\",\"value\":%d}",
                          g_iot_controller.local_gpios[i].name,
                          g_iot_controller.local_gpio_states[i]);
    }

    offset += snprintf(buffer + offset, buffer_size - offset,
                       "],\"remote_devices\":[");

    for (int d = 0; d < g_iot_controller.remote_device_count && offset < buffer_size - 100; d++) {
        if (d > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");

        const char* display_name = g_iot_controller.remote_devices[d].device_name;
        if (strcmp(display_name, "一楼设备") == 0) {
            display_name = "一楼";
        } else if (strcmp(display_name, "二楼设备") == 0) {
            display_name = "二楼";
        } else if (strcmp(display_name, "三楼设备") == 0) {
            display_name = "三楼";
        }

        offset += snprintf(buffer + offset, buffer_size - offset,
                          "{\"name\":\"%s\",\"online\":%s,\"gpio_count\":%d,\"gpios\":[",
                          display_name,
                          g_iot_controller.remote_devices[d].online ? "true" : "false",
                          g_iot_controller.remote_devices[d].gpio_count);

        for (int g = 0; g < g_iot_controller.remote_devices[d].gpio_count && offset < buffer_size - 50; g++) {
            if (g > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");
            offset += snprintf(buffer + offset, buffer_size - offset,
                              "{\"index\":%d,\"value\":%d}",
                              g, g_iot_controller.remote_devices[d].gpio_states[g]);
        }

        offset += snprintf(buffer + offset, buffer_size - offset, "]}");
    }

    offset += snprintf(buffer + offset, buffer_size - offset, "]}");
}

bool App_IOT_Scenario_Homecoming(const char* first_floor_device, const char* all_floors_device) {
    ESP_LOGI(TAG, "[SCENARIO] Homecoming started...");
    bool success = true;
    if (first_floor_device != NULL) {
        int device_index = IOT_Find_Device_By_Name(&g_iot_controller, first_floor_device);
        if (device_index >= 0) {
            if (!IOT_Set_Servo(&g_iot_controller, device_index, 0, 180)) {
                ESP_LOGW(TAG, "Failed to open door");
                success = false;
            }
            vTaskDelay(pdMS_TO_TICKS(500));
        } else {
            ESP_LOGW(TAG, "Device '%s' not found for door control", first_floor_device);
            success = false;
        }
    }
    success = IOT_Broadcast_Lights_On(&g_iot_controller) && success;
    ESP_LOGI(TAG, "[SCENARIO] Homecoming completed: door open, lights on");
    return success;
}

bool App_IOT_Scenario_LeaveHome(const char* first_floor_device) {
    ESP_LOGI(TAG, "[SCENARIO] LeaveHome - closing door...");
    if (first_floor_device == NULL) {
        return false;
    }
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, first_floor_device);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", first_floor_device);
        return false;
    }
    if (!IOT_Set_Servo(&g_iot_controller, device_index, 0, 0)) {
        ESP_LOGW(TAG, "Failed to close door");
        return false;
    }
    ESP_LOGI(TAG, "[SCENARIO] LeaveHome completed: door closed");
    return true;
}

bool App_IOT_Scenario_GoodNight(const char* all_floors_device) {
    ESP_LOGI(TAG, "[SCENARIO] GoodNight: turning off all lights...");
    return IOT_Broadcast_Lights_Off(&g_iot_controller);
}

bool App_IOT_Scenario_Emergency(void) {
    ESP_LOGW(TAG, "[EMERGENCY] Triggering emergency protocol!");
    iot_command_packet_t cmd = {
        .command = IOT_CMD_EMERGENCY,
        .device_id = 0,
        .gpio_index = 0,
        .value = 1,
        .reserved = {0}
    };
    if (g_iot_controller.Mqtt_Publish) {
        bool result = g_iot_controller.Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        ESP_LOGW(TAG, "[EMERGENCY] Emergency broadcast sent, result=%d", result);
        return result;
    }
    ESP_LOGE(TAG, "[EMERGENCY] Mqtt_Publish not initialized!");
    return false;
}

bool App_IOT_Fire_Reset(void) {
    ESP_LOGI(TAG, "[FIRE_RESET] Resetting fire status to NORMAL");
    iot_command_packet_t cmd = {
        .command = IOT_CMD_FIRE_RESET,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };
    if (g_iot_controller.Mqtt_Publish) {
        bool result = g_iot_controller.Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        ESP_LOGI(TAG, "[FIRE_RESET] Fire reset broadcast sent, result=%d", result);
        return result;
    }
    return false;
}

bool App_IOT_Control_Servo_Angle(const char* device_name, uint8_t servo_index, uint8_t angle) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, device_name);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", device_name);
        return false;
    }
    ESP_LOGI(TAG, "Set Servo[%d] angle=%d on %s", servo_index, angle, device_name);
    return IOT_Set_Servo(&g_iot_controller, device_index, servo_index, angle);
}

bool App_IOT_Scenario_Rain_Collect_Clothes(const char* third_floor_device) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, third_floor_device);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", third_floor_device);
        return false;
    }
    ESP_LOGI(TAG, "[SCENARIO] Rain: collecting clothes, closing all servos on 3rd floor");
    bool success = true;
    for (int i = 0; i < 3; i++) {
        if (!IOT_Set_Servo(&g_iot_controller, device_index, i, 0)) {
            success = false;
        }
    }
    return success;
}

bool App_IOT_Scenario_Ventilate(const char* third_floor_device, uint8_t window_angle, uint8_t fan_on) {
    int device_index = IOT_Find_Device_By_Name(&g_iot_controller, third_floor_device);
    if (device_index < 0) {
        ESP_LOGW(TAG, "Device '%s' not found", third_floor_device);
        return false;
    }
    ESP_LOGI(TAG, "[SCENARIO] Ventilate: window=%d deg, fan=%s",
             window_angle, fan_on ? "ON" : "OFF");
    IOT_Set_Servo(&g_iot_controller, device_index, 0, window_angle);
    IOT_Set_Servo(&g_iot_controller, device_index, 1, window_angle);
    IOT_Set_Relay(&g_iot_controller, device_index, 0, fan_on);
    return true;
}

void App_IOT_Get_Sensors_Json(char* buffer, int buffer_size) {
    IOT_Get_Sensors_Json(&g_iot_controller, buffer, buffer_size);
}

void App_IOT_Get_Event_Log_Json(char* buffer, int buffer_size) {
    IOT_Get_Event_Log_Json(&g_iot_controller, buffer, buffer_size);
}

bool App_IOT_MQTT_Publish(const char* topic, const char* data, int len) {
    if (s_mqtt_client == NULL) return false;
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, data, len, 1, 0);
    return msg_id >= 0;
}

bool App_IOT_MQTT_Subscribe(const char* topic) {
    if (s_mqtt_client == NULL) return false;
    int msg_id = esp_mqtt_client_subscribe(s_mqtt_client, topic, 0);
    return msg_id >= 0;
}

void App_IOT_Register_Mqtt_Cmd_Callback(const char* topic_prefix, iot_mqtt_cmd_callback_t callback) {
    if (s_mqtt_cmd_callback_count >= MAX_MQTT_CMD_CALLBACKS) return;
    if (topic_prefix == NULL || callback == NULL) return;
    strncpy(s_mqtt_cmd_callbacks[s_mqtt_cmd_callback_count].topic_prefix,
            topic_prefix, sizeof(s_mqtt_cmd_callbacks[0].topic_prefix) - 1);
    s_mqtt_cmd_callbacks[s_mqtt_cmd_callback_count].topic_prefix[
        sizeof(s_mqtt_cmd_callbacks[0].topic_prefix) - 1] = '\0';
    s_mqtt_cmd_callbacks[s_mqtt_cmd_callback_count].callback = callback;
    s_mqtt_cmd_callback_count++;
    ESP_LOGI(TAG, "Registered MQTT cmd callback for prefix: %s", topic_prefix);
}

static iot_emergency_alert_cb_t s_emergency_alert_cb = NULL;

void App_IOT_Register_Emergency_Alert_Callback(iot_emergency_alert_cb_t cb) {
    s_emergency_alert_cb = cb;
}

void App_IOT_Emergency_Alert(const char* type, const char* detail) {
    ESP_LOGW(TAG, "=== EMERGENCY ALERT: type=%s, detail=%s ===", type, detail);

    IOT_Log_Event(&g_iot_controller, "紧急事件: %s - %s", type, detail);

    if (strcmp(type, "fire") == 0) {
        App_IOT_Scenario_Emergency();
    }

    if (s_emergency_alert_cb != NULL) {
        s_emergency_alert_cb(type, detail);
    }
}
