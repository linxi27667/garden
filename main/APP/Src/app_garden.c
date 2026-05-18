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
#define SERVO_GPIO_INDEX_BASE   7

#define GARDEN_MQTT_TOPIC_STATUS   "xiaozhi/garden/status"
#define GARDEN_MQTT_TOPIC_SENSORS  "xiaozhi/garden/sensors"
#define GARDEN_MQTT_TOPIC_CMD      "xiaozhi/garden/cmd"

static garden_device_state_t g_expected_state;
static garden_device_state_t g_actual_state;
static garden_sensor_data_t  g_sensor_data;
static bool g_garden_online = false;

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

static void Garden_Update_Actual_By_Gpio(uint8_t gpio_index, uint8_t value, uint8_t original_cmd) {
    g_garden_online = true;

    if (original_cmd == IOT_CMD_SET_AUTO_MODE) {
        g_actual_state.auto_mode = (value == 1);
        return;
    }

    if (original_cmd == IOT_CMD_FIRE_RESET) {
        g_actual_state.fire_status = GARDEN_FIRE_NORMAL;
        return;
    }

    switch (gpio_index) {
        case 0:
            g_actual_state.corridor_light = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 1:
            g_actual_state.strip_light = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 2:
            g_actual_state.veg_light = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 3:
            g_actual_state.fan = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 4:
            g_actual_state.water_pump = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 5:
            g_actual_state.atomizer = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 6:
            g_actual_state.safety_switch = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 7:
            g_actual_state.servo = (value >= 90) ? GARDEN_SERVO_HANG : GARDEN_SERVO_COLLECT;
            break;
    }
}

static void Garden_Sensor_Callback(const char* topic, const char* data, int data_len) {
    if (data == NULL || data_len <= 0) return;

    if (strstr(topic, MQTT_TOPIC_EMERGENCY_PREFIX) != NULL) {
        g_garden_online = true;
        cJSON* root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON* type_item = cJSON_GetObjectItem(root, "emergency_type");
            int etype = type_item ? type_item->valueint : -1;
            if (etype == 0) {
                g_sensor_data.fire_status = GARDEN_FIRE_CONFIRMED;
                g_actual_state.fire_status = GARDEN_FIRE_CONFIRMED;
            }
            cJSON_Delete(root);
        }
        return;
    }

    if (strstr(topic, MQTT_TOPIC_RESP_PREFIX) != NULL &&
        data_len >= (int)sizeof(iot_command_packet_t)) {
        const iot_command_packet_t* pkt = (const iot_command_packet_t*)data;
        if (pkt->command == IOT_CMD_RESPONSE && pkt->device_id == GARDEN_DEVICE_ID) {
            Garden_Update_Actual_By_Gpio(pkt->gpio_index, pkt->value, pkt->reserved[0]);
        }
        return;
    }

    if (strstr(topic, MQTT_TOPIC_SENSOR_PREFIX) != NULL && data[0] == '{') {
        g_garden_online = true;
        cJSON* root = cJSON_ParseWithLength(data, data_len);
        if (root) {
            cJSON* item;
            item = cJSON_GetObjectItem(root, "soil_mv");
            if (item) {
                uint16_t mv = (uint16_t)item->valueint;
                g_sensor_data.soil_percent = (mv * 100) / 3100;
                if (g_sensor_data.soil_percent > 100) g_sensor_data.soil_percent = 100;
            }
            item = cJSON_GetObjectItem(root, "rain_mv");
            if (item) g_sensor_data.rain_adc_mv = (uint16_t)item->valueint;
            item = cJSON_GetObjectItem(root, "fire_status");
            if (item) {
                g_sensor_data.fire_status = (garden_fire_state_t)item->valueint;
                g_actual_state.fire_status = g_sensor_data.fire_status;
            }
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
        g_garden_online = true;
        if (data_len >= (int)sizeof(iot_command_packet_t)) {
            const iot_command_packet_t* pkt = (const iot_command_packet_t*)data;
            switch (pkt->gpio_index) {
                case IOT_SENSOR_SOIL_MV: {
                    uint16_t mv = pkt->value | (pkt->reserved[0] << 8);
                    g_sensor_data.soil_percent = (mv * 100) / 3100;
                    if (g_sensor_data.soil_percent > 100) g_sensor_data.soil_percent = 100;
                    break;
                }
                case IOT_SENSOR_RAIN_ADC_MV: {
                    g_sensor_data.rain_adc_mv = pkt->value | (pkt->reserved[0] << 8);
                    break;
                }
                case IOT_SENSOR_FIRE_STATUS:
                    g_sensor_data.fire_status = (garden_fire_state_t)pkt->value;
                    g_actual_state.fire_status = g_sensor_data.fire_status;
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

    g_expected_state = (garden_device_state_t){
        .corridor_light = GARDEN_SWITCH_OFF,
        .strip_light = GARDEN_SWITCH_OFF,
        .veg_light = GARDEN_SWITCH_OFF,
        .veg_pwm_duty = 0,
        .fan = GARDEN_SWITCH_OFF,
        .servo = GARDEN_SERVO_HANG,
        .water_pump = GARDEN_SWITCH_OFF,
        .atomizer = GARDEN_SWITCH_OFF,
        .safety_switch = GARDEN_SWITCH_OFF,
        .auto_mode = true,
        .fire_status = GARDEN_FIRE_NORMAL,
    };
    g_actual_state = g_expected_state;
    g_garden_online = false;

    memset(&g_sensor_data, 0, sizeof(g_sensor_data));

    ESP_LOGI(TAG, "Garden Controller initialized (all devices on slave, master sends MQTT commands)");
}

void App_Garden_Start(void) {
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/garden/", Garden_MQTT_Cmd_Callback);
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/iot/sensor/", Garden_Sensor_Callback);
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/iot/emergency/", Garden_Sensor_Callback);
    App_IOT_Register_Mqtt_Cmd_Callback("xiaozhi/iot/resp/", Garden_Sensor_Callback);
    ESP_LOGI(TAG, "Garden MQTT command and sensor callbacks registered");
}

garden_device_state_t App_Garden_Get_State(void) {
    return g_actual_state;
}

garden_sensor_data_t App_Garden_Get_Sensors(void) {
    return g_sensor_data;
}

void App_Garden_Set_Corridor_Light(garden_switch_state_t state) {
    g_expected_state.corridor_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 0, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Corridor light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Strip_Light(garden_switch_state_t state) {
    g_expected_state.strip_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 1, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Strip light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Veg_Light(garden_switch_state_t state) {
    g_expected_state.veg_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 2, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Veg light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Fan(garden_switch_state_t state) {
    g_expected_state.fan = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 0, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Fan: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Servo(garden_servo_state_t state) {
    g_expected_state.servo = state;
    Send_IoT_Command(IOT_CMD_SET_SERVO, SERVO_GPIO_INDEX_BASE, state == GARDEN_SERVO_HANG ? 180 : 0);
    ESP_LOGI(TAG, "Servo: %s (MQTT->slave)", state ? "HANG" : "COLLECT");
}

void App_Garden_Set_Auto_Mode(bool enable) {
    g_expected_state.auto_mode = enable;
    Send_IoT_Command(IOT_CMD_SET_AUTO_MODE, 0, enable ? 1 : 0);
    ESP_LOGI(TAG, "Auto mode: %s (MQTT->slave)", enable ? "ON" : "OFF");
}

void App_Garden_Set_Water_Pump(garden_switch_state_t state) {
    g_expected_state.water_pump = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 1, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Water pump: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Atomizer(garden_switch_state_t state) {
    g_expected_state.atomizer = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 2, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Atomizer: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Safety_Switch(garden_switch_state_t state) {
    g_expected_state.safety_switch = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 3, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Safety switch: %s (MQTT->slave)", state ? "TRIPPED" : "NORMAL");
}

void App_Garden_All_Lights_On(void) {
    App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
    App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
    App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON);
}

void App_Garden_All_Lights_Off(void) {
    App_Garden_Set_Corridor_Light(GARDEN_SWITCH_OFF);
    App_Garden_Set_Strip_Light(GARDEN_SWITCH_OFF);
    App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF);
}

void App_Garden_Fire_Emergency_Response(void) {
    ESP_LOGW(TAG, "=== FIRE EMERGENCY RESPONSE (MQTT broadcast) ===");
    g_expected_state.fire_status = GARDEN_FIRE_CONFIRMED;

    App_IOT_Scenario_Emergency();

    ESP_LOGW(TAG, "Fire emergency: EMERGENCY broadcast sent to all slaves");
}

void App_Garden_Get_Status_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"online\":%s,"
        "\"expected\":{\"corridor_light\":%d,\"strip_light\":%d,\"veg_light\":%d,"
        "\"veg_pwm_duty\":%d,\"fan\":%d,\"servo\":%d,\"auto_mode\":%s,"
        "\"water_pump\":%d,\"atomizer\":%d,\"safety_switch\":%d,\"fire_status\":%d},"
        "\"actual\":{\"corridor_light\":%d,\"strip_light\":%d,\"veg_light\":%d,"
        "\"veg_pwm_duty\":%d,\"fan\":%d,\"servo\":%d,\"auto_mode\":%s,"
        "\"water_pump\":%d,\"atomizer\":%d,\"safety_switch\":%d,\"fire_status\":%d}}",
        g_garden_online ? "true" : "false",
        g_expected_state.corridor_light,
        g_expected_state.strip_light,
        g_expected_state.veg_light,
        g_expected_state.veg_pwm_duty,
        g_expected_state.fan,
        g_expected_state.servo,
        g_expected_state.auto_mode ? "true" : "false",
        g_expected_state.water_pump,
        g_expected_state.atomizer,
        g_expected_state.safety_switch,
        g_expected_state.fire_status,
        g_actual_state.corridor_light,
        g_actual_state.strip_light,
        g_actual_state.veg_light,
        g_actual_state.veg_pwm_duty,
        g_actual_state.fan,
        g_actual_state.servo,
        g_actual_state.auto_mode ? "true" : "false",
        g_actual_state.water_pump,
        g_actual_state.atomizer,
        g_actual_state.safety_switch,
        g_actual_state.fire_status);
}

void App_Garden_Get_Sensors_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"soil\":%d,\"rain\":%d,\"rain_adc_mv\":%d,\"rain_state\":\"%s\","
        "\"fire_status\":%d,\"help_status\":%d,"
        "\"temperature\":%.1f,\"humidity\":%.1f}",
        g_sensor_data.soil_percent,
        g_sensor_data.rain_state,
        g_sensor_data.rain_adc_mv,
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
            if (strcmp(act, "on") == 0) { App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Corridor_Light(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "strip_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Strip_Light(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "veg_light") == 0 || strcmp(dev_name, "corridor_long_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "fan") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Fan(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Fan(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "servo") == 0) {
            if (strcmp(act, "hang") == 0) { App_Garden_Set_Servo(GARDEN_SERVO_HANG); success = true; }
            else if (strcmp(act, "collect") == 0) { App_Garden_Set_Servo(GARDEN_SERVO_COLLECT); success = true; }
        } else if (strcmp(dev_name, "water_pump") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Water_Pump(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "atomizer") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Atomizer(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "safety_switch") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Safety_Switch(GARDEN_SWITCH_OFF); success = true; }
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
