#include "app_garden.h"
#include "iot_controller.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>

static const char* TAG = "APP_GARDEN";

#define GARDEN_DEVICE_ID        10
#define GARDEN_DEVICE_NAME      "智慧庭院"

#define GARDEN_MQTT_TOPIC_STATUS   "xiaozhi/garden/status"
#define GARDEN_MQTT_TOPIC_SENSORS  "xiaozhi/garden/sensors"
#define GARDEN_MQTT_TOPIC_CMD      "xiaozhi/garden/cmd"

static garden_device_state_t g_device_state;
static garden_sensor_data_t  g_sensor_data;

static bool Send_IoT_Command(uint8_t command, uint8_t gpio_index, uint8_t value) {
    iot_command_packet_t pkt = {
        .command = command,
        .device_id = GARDEN_DEVICE_ID,
        .gpio_index = gpio_index,
        .value = value,
        .reserved = {0}
    };

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_CMD_PREFIX "%d", GARDEN_DEVICE_ID);

    return App_IOT_MQTT_Publish(topic, (const char*)&pkt, sizeof(pkt));
}

static void Garden_MQTT_Cmd_Callback(const char* topic, const char* data, int data_len) {
    ESP_LOGI(TAG, "MQTT garden cmd received: %.*s", data_len, data);
    App_Garden_Process_Mqtt_Cmd(data, data_len);
}

static void Garden_Sensor_Callback(const char* topic, const char* data, int data_len) {
    if (data == NULL || data_len <= 0) return;

    if (strstr(topic, MQTT_TOPIC_EMERGENCY_PREFIX) != NULL) {
        cJSON* root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON* type_item = cJSON_GetObjectItem(root, "emergency_type");
            int etype = type_item ? type_item->valueint : -1;
            if (etype == 0) {
                g_sensor_data.fire_status = GARDEN_FIRE_CONFIRMED;
            }
            cJSON_Delete(root);
        }
        return;
    }

    if (strstr(topic, MQTT_TOPIC_SENSOR_PREFIX) != NULL && data[0] == '{') {
        cJSON* root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON* item;
            item = cJSON_GetObjectItem(root, "soil_mv");
            if (item) {
                uint16_t mv = (uint16_t)item->valueint;
                g_sensor_data.soil_value = (mv * 100) / 3100;
                if (g_sensor_data.soil_value > 100) g_sensor_data.soil_value = 100;
            }
            item = cJSON_GetObjectItem(root, "light_mv");
            if (item) {
                uint16_t mv = (uint16_t)item->valueint;
                g_sensor_data.light_value = (mv * 100) / 3100;
                if (g_sensor_data.light_value > 100) g_sensor_data.light_value = 100;
            }
            item = cJSON_GetObjectItem(root, "fire_status");
            if (item) g_sensor_data.fire_status = (garden_fire_state_t)item->valueint;
            item = cJSON_GetObjectItem(root, "rain_status");
            if (item) g_sensor_data.rain_state = (item->valueint == 1) ? GARDEN_RAIN_WET : GARDEN_RAIN_DRY;
            item = cJSON_GetObjectItem(root, "help_status");
            if (item) g_sensor_data.help_status = (uint8_t)item->valueint;
            item = cJSON_GetObjectItem(root, "temperature");
            if (item) g_sensor_data.temperature = (float)item->valuedouble;
            item = cJSON_GetObjectItem(root, "humidity");
            if (item) g_sensor_data.humidity = (float)item->valuedouble;
            cJSON_Delete(root);
        }
        return;
    }

    if (strstr(topic, MQTT_TOPIC_SENSOR_PREFIX) != NULL) {
        if (data_len >= (int)sizeof(iot_command_packet_t)) {
            const iot_command_packet_t* pkt = (const iot_command_packet_t*)data;
            switch (pkt->gpio_index) {
                case IOT_SENSOR_SMOKE_MV: {
                    uint16_t mv = pkt->value | (pkt->reserved[0] << 8);
                    g_sensor_data.soil_value = (mv * 100) / 3100;
                    if (g_sensor_data.soil_value > 100) g_sensor_data.soil_value = 100;
                    break;
                }
                case IOT_SENSOR_RAIN_MV: {
                    uint16_t mv = pkt->value | (pkt->reserved[0] << 8);
                    g_sensor_data.light_value = (mv * 100) / 3100;
                    if (g_sensor_data.light_value > 100) g_sensor_data.light_value = 100;
                    break;
                }
                case IOT_SENSOR_FIRE_STATUS:
                    g_sensor_data.fire_status = (garden_fire_state_t)pkt->value;
                    break;
                case IOT_SENSOR_RAIN_STATUS:
                    g_sensor_data.rain_state = (pkt->value == 1) ? GARDEN_RAIN_WET : GARDEN_RAIN_DRY;
                    break;
                case IOT_SENSOR_HELP_STATUS:
                    g_sensor_data.help_status = pkt->value;
                    break;
            }
        }
    }
}

void App_Garden_Init(void) {
    ESP_LOGI(TAG, "Initializing Garden Controller (MQTT command mode)...");

    g_device_state = (garden_device_state_t){
        .corridor_light = GARDEN_LIGHT_OFF,
        .strip_light = GARDEN_LIGHT_OFF,
        .veg_light = GARDEN_LIGHT_OFF,
        .veg_pwm_duty = 0,
        .fan = GARDEN_LIGHT_OFF,
        .servo = GARDEN_SERVO_HANG,
        .water_pump = GARDEN_LIGHT_OFF,
        .atomizer = GARDEN_LIGHT_OFF,
        .safety_switch = GARDEN_LIGHT_OFF,
        .auto_mode = true,
        .fire_status = GARDEN_FIRE_NORMAL,
    };

    memset(&g_sensor_data, 0, sizeof(g_sensor_data));

    ESP_LOGI(TAG, "Garden Controller initialized (all devices on slave, master sends MQTT commands)");
}

void App_Garden_Start(void) {
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/garden/", Garden_MQTT_Cmd_Callback);
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/iot/sensor/", Garden_Sensor_Callback);
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/iot/emergency/", Garden_Sensor_Callback);
    ESP_LOGI(TAG, "Garden MQTT command and sensor callbacks registered");
}

garden_device_state_t App_Garden_Get_State(void) {
    return g_device_state;
}

garden_sensor_data_t App_Garden_Get_Sensors(void) {
    return g_sensor_data;
}

void App_Garden_Set_Corridor_Light(garden_light_state_t state) {
    g_device_state.corridor_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 0, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Corridor light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Strip_Light(garden_light_state_t state) {
    g_device_state.strip_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 1, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Strip light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Veg_Light(garden_light_state_t state) {
    g_device_state.veg_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 2, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Veg light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Fan(garden_light_state_t state) {
    g_device_state.fan = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 0, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Fan: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Servo(garden_servo_state_t state) {
    g_device_state.servo = state;
    Send_IoT_Command(IOT_CMD_SET_SERVO, 6, state == GARDEN_SERVO_HANG ? 180 : 0);
    ESP_LOGI(TAG, "Servo: %s (MQTT->slave)", state ? "HANG" : "COLLECT");
}

void App_Garden_Set_Auto_Mode(bool enable) {
    g_device_state.auto_mode = enable;
    Send_IoT_Command(IOT_CMD_SET_AUTO_MODE, 0, enable ? 1 : 0);
    ESP_LOGI(TAG, "Auto mode: %s (MQTT->slave)", enable ? "ON" : "OFF");
}

void App_Garden_Set_Water_Pump(garden_light_state_t state) {
    g_device_state.water_pump = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 1, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Water pump: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Atomizer(garden_light_state_t state) {
    g_device_state.atomizer = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 2, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Atomizer: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Safety_Switch(garden_light_state_t state) {
    g_device_state.safety_switch = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 3, state == GARDEN_LIGHT_ON ? 1 : 0);
    ESP_LOGI(TAG, "Safety switch: %s (MQTT->slave)", state ? "TRIPPED" : "NORMAL");
}

void App_Garden_All_Lights_On(void) {
    App_Garden_Set_Corridor_Light(GARDEN_LIGHT_ON);
    App_Garden_Set_Strip_Light(GARDEN_LIGHT_ON);
    App_Garden_Set_Veg_Light(GARDEN_LIGHT_ON);
}

void App_Garden_All_Lights_Off(void) {
    App_Garden_Set_Corridor_Light(GARDEN_LIGHT_OFF);
    App_Garden_Set_Strip_Light(GARDEN_LIGHT_OFF);
    App_Garden_Set_Veg_Light(GARDEN_LIGHT_OFF);
}

void App_Garden_Fire_Emergency_Response(void) {
    ESP_LOGW(TAG, "=== FIRE EMERGENCY RESPONSE (MQTT broadcast) ===");
    g_device_state.fire_status = GARDEN_FIRE_CONFIRMED;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_EMERGENCY,
        .device_id = 0,
        .gpio_index = 0,
        .value = 1,
        .reserved = {0}
    };
    App_IOT_MQTT_Publish(MQTT_TOPIC_CMD_BROADCAST, (const char*)&cmd, sizeof(cmd));

    App_IOT_Scenario_Emergency();

    ESP_LOGW(TAG, "Fire emergency: EMERGENCY broadcast sent to all slaves");
}

void App_Garden_Get_Status_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"corridor_light\":%d,\"strip_light\":%d,\"veg_light\":%d,"
        "\"veg_pwm_duty\":%d,\"fan\":%d,\"servo\":%d,\"auto_mode\":%s,"
        "\"water_pump\":%d,\"atomizer\":%d,\"safety_switch\":%d,\"fire_status\":%d}",
        g_device_state.corridor_light,
        g_device_state.strip_light,
        g_device_state.veg_light,
        g_device_state.veg_pwm_duty,
        g_device_state.fan,
        g_device_state.servo,
        g_device_state.auto_mode ? "true" : "false",
        g_device_state.water_pump,
        g_device_state.atomizer,
        g_device_state.safety_switch,
        g_device_state.fire_status);
}

void App_Garden_Get_Sensors_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"soil\":%d,\"light\":%d,\"rain\":%d,\"rain_state\":\"%s\","
        "\"fire_status\":%d,\"help_status\":%d,"
        "\"temperature\":%.1f,\"humidity\":%.1f}",
        g_sensor_data.soil_value,
        g_sensor_data.light_value,
        g_sensor_data.rain_state,
        g_sensor_data.rain_state == GARDEN_RAIN_WET ? "wet" : "dry",
        g_sensor_data.fire_status,
        g_sensor_data.help_status,
        g_sensor_data.temperature,
        g_sensor_data.humidity);
}

bool App_Garden_Process_Mqtt_Cmd(const char* json_cmd, int cmd_len) {
    if (json_cmd == NULL || cmd_len <= 0) return false;

    cJSON* root = cJSON_ParseWithLength(json_cmd, cmd_len);
    if (root == NULL) return false;

    cJSON* device = cJSON_GetObjectItem(root, "device");
    cJSON* action = cJSON_GetObjectItem(root, "action");
    cJSON* value = cJSON_GetObjectItem(root, "value");

    bool success = false;

    if (device != NULL && action != NULL && cJSON_IsString(device) && cJSON_IsString(action)) {
        const char* dev_name = device->valuestring;
        const char* act = action->valuestring;

        if (strcmp(dev_name, "corridor_light") == 0 || strcmp(dev_name, "courtyard_west_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Corridor_Light(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Corridor_Light(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "strip_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Strip_Light(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Strip_Light(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "veg_light") == 0 || strcmp(dev_name, "corridor_long_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Veg_Light(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Veg_Light(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "fan") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Fan(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Fan(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "servo") == 0) {
            if (strcmp(act, "hang") == 0) { App_Garden_Set_Servo(GARDEN_SERVO_HANG); success = true; }
            else if (strcmp(act, "collect") == 0) { App_Garden_Set_Servo(GARDEN_SERVO_COLLECT); success = true; }
        } else if (strcmp(dev_name, "water_pump") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Water_Pump(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Water_Pump(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "atomizer") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Atomizer(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Atomizer(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "safety_switch") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Safety_Switch(GARDEN_LIGHT_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Safety_Switch(GARDEN_LIGHT_OFF); success = true; }
        } else if (strcmp(dev_name, "auto_mode") == 0) {
            if (value != NULL) {
                App_Garden_Set_Auto_Mode(cJSON_IsTrue(value));
                success = true;
            }
        } else if (strcmp(dev_name, "all_lights") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_All_Lights_On(); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_All_Lights_Off(); success = true; }
        } else if (strcmp(dev_name, "emergency") == 0) {
            if (strcmp(act, "fire") == 0) { App_Garden_Fire_Emergency_Response(); success = true; }
        }
    }

    cJSON_Delete(root);
    return success;
}
