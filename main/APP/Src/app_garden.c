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

static bool Garden_Set_Auto_State(garden_device_state_t* state, uint8_t auto_id, bool enable) {
    if (state == NULL) return false;
    switch (auto_id) {
    case IOT_AUTO_DEVICE_FAN:
        state->auto_fan = enable;
        return true;
    case IOT_AUTO_DEVICE_ATOMIZER:
        state->auto_atomizer = enable;
        return true;
    case IOT_AUTO_DEVICE_WATERING:
        state->auto_watering = enable;
        return true;
    case IOT_AUTO_DEVICE_RAIN_SHELTER:
        state->auto_rain_shelter = enable;
        return true;
    default:
        return false;
    }
}

static bool Garden_Auto_Id_From_Target(const char* target, uint8_t* auto_id) {
    if (target == NULL || auto_id == NULL) return false;
    if (strcmp(target, "fan") == 0 || strcmp(target, "temperature") == 0) {
        *auto_id = IOT_AUTO_DEVICE_FAN;
        return true;
    }
    if (strcmp(target, "atomizer") == 0 || strcmp(target, "humidity") == 0 || strcmp(target, "mist") == 0) {
        *auto_id = IOT_AUTO_DEVICE_ATOMIZER;
        return true;
    }
    if (strcmp(target, "watering") == 0 || strcmp(target, "water_pump") == 0 || strcmp(target, "soil") == 0) {
        *auto_id = IOT_AUTO_DEVICE_WATERING;
        return true;
    }
    if (strcmp(target, "rain_shelter") == 0 || strcmp(target, "servo") == 0 || strcmp(target, "canopy") == 0) {
        *auto_id = IOT_AUTO_DEVICE_RAIN_SHELTER;
        return true;
    }
    return false;
}

static bool Send_Strip_RGB_Command(uint8_t red, uint8_t green, uint8_t blue,
                                   uint8_t brightness, uint8_t effect) {
    if (brightness > 100) {
        brightness = 100;
    }

    iot_command_packet_t pkt = {
        .command = IOT_CMD_SET_STRIP_RGB,
        .device_id = GARDEN_DEVICE_ID,
        .gpio_index = 1,
        .value = brightness,
        .reserved = {red, green, blue, effect}
    };

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_CMD_PREFIX "%d", GARDEN_DEVICE_ID);

    return App_IOT_MQTT_Publish(topic, (const char*)&pkt, sizeof(pkt));
}

static uint8_t Garden_Scene_Id_From_Name(const char* scene) {
    if (scene == NULL) return IOT_AMBIENCE_SCENE_CUSTOM;
    if (strcmp(scene, "normal") == 0 || strcmp(scene, "default") == 0 ||
        strcmp(scene, "恢复正常") == 0 || strcmp(scene, "正常") == 0 ||
        strcmp(scene, "风和日丽") == 0) {
        return IOT_AMBIENCE_SCENE_NORMAL;
    }
    if (strcmp(scene, "night") == 0 || strcmp(scene, "夜间") == 0 || strcmp(scene, "夜间模式") == 0 ||
        strcmp(scene, "月白风清") == 0) {
        return IOT_AMBIENCE_SCENE_NIGHT;
    }
    if (strcmp(scene, "hot") == 0 || strcmp(scene, "cool") == 0 || strcmp(scene, "闷热") == 0 || strcmp(scene, "降温") == 0 ||
        strcmp(scene, "清风徐来") == 0) {
        return IOT_AMBIENCE_SCENE_HOT;
    }
    if (strcmp(scene, "relax") == 0 || strcmp(scene, "休闲") == 0 || strcmp(scene, "放松") == 0 ||
        strcmp(scene, "闲庭信步") == 0) {
        return IOT_AMBIENCE_SCENE_RELAX;
    }
    if (strcmp(scene, "rain") == 0 || strcmp(scene, "雨天") == 0 ||
        strcmp(scene, "烟雨朦胧") == 0) {
        return IOT_AMBIENCE_SCENE_RAIN;
    }
    if (strcmp(scene, "safe") == 0 || strcmp(scene, "security") == 0 || strcmp(scene, "安全") == 0) {
        return IOT_AMBIENCE_SCENE_SAFE;
    }
    if (strcmp(scene, "eco") == 0 || strcmp(scene, "low_power") == 0 || strcmp(scene, "节能") == 0 || strcmp(scene, "低功耗") == 0 ||
        strcmp(scene, "万籁俱寂") == 0) {
        return IOT_AMBIENCE_SCENE_ECO;
    }
    return IOT_AMBIENCE_SCENE_CUSTOM;
}

static const char* Garden_Scene_Name_From_Id(uint8_t scene_id) {
    switch (scene_id) {
    case IOT_AMBIENCE_SCENE_NORMAL: return "风和日丽";
    case IOT_AMBIENCE_SCENE_NIGHT: return "月白风清";
    case IOT_AMBIENCE_SCENE_HOT: return "清风徐来";
    case IOT_AMBIENCE_SCENE_RELAX: return "闲庭信步";
    case IOT_AMBIENCE_SCENE_RAIN: return "烟雨朦胧";
    case IOT_AMBIENCE_SCENE_SAFE: return "安全警戒";
    case IOT_AMBIENCE_SCENE_ECO: return "万籁俱寂";
    case IOT_AMBIENCE_SCENE_CUSTOM:
    default:
        return "custom";
    }
}

static void Garden_MQTT_Cmd_Callback(const char* topic, const char* data, int data_len) {
    ESP_LOGI(TAG, "MQTT garden cmd received: %.*s", data_len, data);
    App_Garden_Process_Mqtt_Cmd(data, data_len);
}

static void Garden_Update_Actual_By_Gpio(uint8_t gpio_index, uint8_t value, uint8_t original_cmd) {
    g_garden_online = true;

    if (original_cmd == IOT_CMD_SET_AUTO_MODE) {
        g_actual_state.auto_mode = (value == 1);
        if (g_actual_state.auto_mode) {
            g_actual_state.auto_fan = true;
            g_actual_state.auto_atomizer = true;
            g_actual_state.auto_watering = true;
            g_actual_state.auto_rain_shelter = true;
        }
        return;
    }

    if (original_cmd == IOT_CMD_SET_DEVICE_AUTO) {
        Garden_Set_Auto_State(&g_actual_state, gpio_index, value == 1);
        return;
    }

    if (original_cmd == IOT_CMD_FIRE_RESET) {
        g_actual_state.fire_status = GARDEN_FIRE_NORMAL;
        g_actual_state.safety_switch = GARDEN_SWITCH_ON;
        g_actual_state.strip_light = GARDEN_SWITCH_ON;
        g_actual_state.strip_red = 255;
        g_actual_state.strip_green = 185;
        g_actual_state.strip_blue = 110;
        g_actual_state.strip_brightness = 35;
        g_actual_state.strip_effect = IOT_STRIP_EFFECT_SOLID;
        g_actual_state.ambience_scene = IOT_AMBIENCE_SCENE_NORMAL;
        return;
    }

    if (original_cmd == IOT_CMD_SET_STRIP_RGB || original_cmd == IOT_CMD_SET_AMBIENCE_SCENE) {
        g_actual_state.strip_light = g_expected_state.strip_light;
        g_actual_state.strip_red = g_expected_state.strip_red;
        g_actual_state.strip_green = g_expected_state.strip_green;
        g_actual_state.strip_blue = g_expected_state.strip_blue;
        g_actual_state.strip_brightness = g_expected_state.strip_brightness;
        g_actual_state.strip_effect = g_expected_state.strip_effect;
        g_actual_state.ambience_scene = g_expected_state.ambience_scene;
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
            if (original_cmd == IOT_CMD_SET_RELAY || original_cmd == IOT_CMD_SET_GPIO) {
                g_actual_state.auto_fan = false;
            }
            break;
        case 4:
            g_actual_state.water_pump = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            if (original_cmd == IOT_CMD_SET_RELAY || original_cmd == IOT_CMD_SET_GPIO) {
                g_actual_state.auto_watering = false;
            }
            break;
        case 5:
            g_actual_state.atomizer = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            if (original_cmd == IOT_CMD_SET_RELAY || original_cmd == IOT_CMD_SET_GPIO) {
                g_actual_state.auto_atomizer = false;
            }
            break;
        case 6:
            g_actual_state.safety_switch = value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
            break;
        case 7:
            g_actual_state.servo = (value >= 45) ? GARDEN_SERVO_HANG : GARDEN_SERVO_COLLECT;
            if (original_cmd == IOT_CMD_SET_SERVO || original_cmd == IOT_CMD_SET_GPIO) {
                g_actual_state.auto_rain_shelter = false;
            }
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
                g_actual_state.safety_switch = GARDEN_SWITCH_OFF;
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
            item = cJSON_GetObjectItem(root, "soil_percent");
            if (item) g_sensor_data.soil_percent = (uint8_t)item->valueint;
            item = cJSON_GetObjectItem(root, "rain_mv");
            if (item) g_sensor_data.rain_adc_mv = (uint16_t)item->valueint;
            item = cJSON_GetObjectItem(root, "rain_percent");
            if (item) g_sensor_data.rain_percent = (uint8_t)item->valueint;
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

            char buf[768];
            App_Garden_Get_Sensors_Json(buf, sizeof(buf));
            App_IOT_MQTT_Publish(GARDEN_MQTT_TOPIC_SENSORS, buf, strlen(buf));
            App_Garden_Get_Status_Json(buf, sizeof(buf));
            App_IOT_MQTT_Publish(GARDEN_MQTT_TOPIC_STATUS, buf, strlen(buf));
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
                    g_sensor_data.soil_percent = 100 - (mv * 100) / 3100;
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
        .strip_red = 255,
        .strip_green = 185,
        .strip_blue = 110,
        .strip_brightness = 35,
        .strip_effect = IOT_STRIP_EFFECT_SOLID,
        .ambience_scene = IOT_AMBIENCE_SCENE_NORMAL,
        .fan = GARDEN_SWITCH_OFF,
        .servo = GARDEN_SERVO_HANG,
        .water_pump = GARDEN_SWITCH_OFF,
        .atomizer = GARDEN_SWITCH_OFF,
        .safety_switch = GARDEN_SWITCH_ON,
        .auto_mode = false,
        .auto_fan = false,
        .auto_atomizer = false,
        .auto_watering = false,
        .auto_rain_shelter = false,
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

bool App_Garden_Set_Strip_RGB(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness, uint8_t effect) {
    if (brightness > 100) {
        brightness = 100;
    }

    g_expected_state.strip_light = brightness > 0 ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF;
    g_expected_state.strip_red = red;
    g_expected_state.strip_green = green;
    g_expected_state.strip_blue = blue;
    g_expected_state.strip_brightness = brightness;
    g_expected_state.strip_effect = effect;
    g_expected_state.ambience_scene = IOT_AMBIENCE_SCENE_CUSTOM;

    bool ok = Send_Strip_RGB_Command(red, green, blue, brightness, effect);
    ESP_LOGI(TAG, "Strip RGB: r=%d g=%d b=%d brightness=%d effect=%d result=%d",
             red, green, blue, brightness, effect, ok);
    return ok;
}

bool App_Garden_Set_Ambience_Scene(const char* scene) {
    uint8_t scene_id = Garden_Scene_Id_From_Name(scene);
    uint8_t red = 255;
    uint8_t green = 185;
    uint8_t blue = 110;
    uint8_t brightness = 35;
    uint8_t effect = IOT_STRIP_EFFECT_SOLID;

    switch (scene_id) {
    case IOT_AMBIENCE_SCENE_NORMAL:  /* 风和日丽 */
        red = 255; green = 195; blue = 80; brightness = 40; effect = IOT_STRIP_EFFECT_SOLID;
        App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON);
        break;
    case IOT_AMBIENCE_SCENE_NIGHT:   /* 月白风清 */
        red = 190; green = 200; blue = 235; brightness = 15; effect = IOT_STRIP_EFFECT_SOLID;
        App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON);
        break;
    case IOT_AMBIENCE_SCENE_HOT:     /* 清风徐来 */
        red = 70; green = 185; blue = 250; brightness = 45; effect = IOT_STRIP_EFFECT_BREATHE;
        App_Garden_Set_Corridor_Light(GARDEN_SWITCH_OFF);
        App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF);
        App_Garden_Set_Fan(GARDEN_SWITCH_ON);
        App_Garden_Set_Atomizer(GARDEN_SWITCH_ON);
        break;
    case IOT_AMBIENCE_SCENE_RELAX:   /* 闲庭信步 */
        red = 65; green = 210; blue = 95; brightness = 28; effect = IOT_STRIP_EFFECT_BREATHE;
        App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF);
        break;
    case IOT_AMBIENCE_SCENE_RAIN:    /* 烟雨朦胧 */
        red = 50; green = 85; blue = 200; brightness = 25; effect = IOT_STRIP_EFFECT_BREATHE;
        App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
        App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON);
        break;
    case IOT_AMBIENCE_SCENE_SAFE:    /* 安全警戒 */
        red = 255; green = 255; blue = 255; brightness = 100; effect = IOT_STRIP_EFFECT_ALERT;
        App_Garden_All_Lights_On();
        App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF);
        App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF);
        App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);
        App_Garden_Set_Auto_Mode(false);
        break;
    case IOT_AMBIENCE_SCENE_ECO:     /* 万籁俱寂 */
        red = 0; green = 0; blue = 0; brightness = 0; effect = IOT_STRIP_EFFECT_SOLID;
        App_Garden_All_Lights_Off();
        break;
    case IOT_AMBIENCE_SCENE_CUSTOM:
    default:
        ESP_LOGW(TAG, "Unknown ambience scene: %s", scene ? scene : "null");
        return false;
    }

    g_expected_state.strip_light = GARDEN_SWITCH_ON;
    g_expected_state.strip_red = red;
    g_expected_state.strip_green = green;
    g_expected_state.strip_blue = blue;
    g_expected_state.strip_brightness = brightness;
    g_expected_state.strip_effect = effect;
    g_expected_state.ambience_scene = scene_id;

    bool ok = Send_Strip_RGB_Command(red, green, blue, brightness, effect);
    Send_IoT_Command(IOT_CMD_SET_AMBIENCE_SCENE, 1, scene_id);
    ESP_LOGI(TAG, "Ambience scene: %s (%d), result=%d", Garden_Scene_Name_From_Id(scene_id), scene_id, ok);
    return ok;
}

void App_Garden_Set_Veg_Light(garden_switch_state_t state) {
    g_expected_state.veg_light = state;
    Send_IoT_Command(IOT_CMD_SET_LIGHT, 2, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Veg light: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Fan(garden_switch_state_t state) {
    g_expected_state.fan = state;
    g_expected_state.auto_fan = false;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 0, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Fan: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Servo(garden_servo_state_t state) {
    g_expected_state.servo = state;
    g_expected_state.auto_rain_shelter = false;
    Send_IoT_Command(IOT_CMD_SET_SERVO, SERVO_GPIO_INDEX_BASE, state == GARDEN_SERVO_HANG ? 90 : 0);
    ESP_LOGI(TAG, "Rain shelter servo: %s (MQTT->slave)", state ? "DEPLOY" : "RETRACT");
}

void App_Garden_Set_Auto_Mode(bool enable) {
    g_expected_state.auto_mode = enable;
    if (enable) {
        g_expected_state.auto_fan = true;
        g_expected_state.auto_atomizer = true;
        g_expected_state.auto_watering = true;
        g_expected_state.auto_rain_shelter = true;
    }
    Send_IoT_Command(IOT_CMD_SET_AUTO_MODE, 0, enable ? 1 : 0);
    ESP_LOGI(TAG, "Auto mode: %s (MQTT->slave)", enable ? "ON" : "OFF");
}

bool App_Garden_Set_Device_Auto(const char* target, bool enable) {
    uint8_t auto_id = 0;
    if (!Garden_Auto_Id_From_Target(target, &auto_id)) {
        ESP_LOGW(TAG, "Unknown auto target: %s", target ? target : "null");
        return false;
    }
    Garden_Set_Auto_State(&g_expected_state, auto_id, enable);
    bool ok = Send_IoT_Command(IOT_CMD_SET_DEVICE_AUTO, auto_id, enable ? 1 : 0);
    ESP_LOGI(TAG, "Device auto target=%s id=%d -> %s result=%d",
             target, auto_id, enable ? "ON" : "OFF", ok);
    return ok;
}

void App_Garden_Set_Water_Pump(garden_switch_state_t state) {
    g_expected_state.water_pump = state;
    g_expected_state.auto_watering = false;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 1, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Water pump: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Atomizer(garden_switch_state_t state) {
    g_expected_state.atomizer = state;
    g_expected_state.auto_atomizer = false;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 2, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Atomizer: %s (MQTT->slave)", state ? "ON" : "OFF");
}

void App_Garden_Set_Safety_Switch(garden_switch_state_t state) {
    g_expected_state.safety_switch = state;
    Send_IoT_Command(IOT_CMD_SET_RELAY, 3, state == GARDEN_SWITCH_ON ? 1 : 0);
    ESP_LOGI(TAG, "Safety switch: %s (MQTT->slave)", state ? "NORMAL/POWER_ON" : "CUT_OFF");
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

void App_Garden_All_Devices_On(void) {
    App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);
    App_Garden_All_Lights_On();
    App_Garden_Set_Fan(GARDEN_SWITCH_ON);
    App_Garden_Set_Water_Pump(GARDEN_SWITCH_ON);
    App_Garden_Set_Atomizer(GARDEN_SWITCH_ON);
    ESP_LOGI(TAG, "All garden devices ON (rain shelter unchanged), safety switch kept NORMAL");
}

void App_Garden_All_Devices_Off(void) {
    App_Garden_All_Lights_Off();
    App_Garden_Set_Fan(GARDEN_SWITCH_OFF);
    App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF);
    App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF);
    App_Garden_Set_Servo(GARDEN_SERVO_COLLECT);
    App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);
    ESP_LOGI(TAG, "All garden user devices OFF, safety switch kept NORMAL");
}

void App_Garden_Fire_Emergency_Response(void) {
    ESP_LOGW(TAG, "=== FIRE EMERGENCY RESPONSE (MASTER + MQTT) ===");
    g_expected_state.fire_status = GARDEN_FIRE_CONFIRMED;
    g_expected_state.safety_switch = GARDEN_SWITCH_OFF;
    g_expected_state.water_pump = GARDEN_SWITCH_OFF;
    g_expected_state.atomizer = GARDEN_SWITCH_OFF;
    g_expected_state.corridor_light = GARDEN_SWITCH_ON;
    g_expected_state.strip_light = GARDEN_SWITCH_ON;
    g_expected_state.veg_light = GARDEN_SWITCH_ON;
    g_expected_state.strip_red = 255;
    g_expected_state.strip_green = 0;
    g_expected_state.strip_blue = 0;
    g_expected_state.strip_brightness = 100;
    g_expected_state.strip_effect = IOT_STRIP_EFFECT_ALERT;
    g_expected_state.ambience_scene = IOT_AMBIENCE_SCENE_SAFE;

    App_Garden_Set_Safety_Switch(GARDEN_SWITCH_OFF);
    App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF);
    App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF);
    App_Garden_All_Lights_On();
    App_Garden_Set_Strip_RGB(255, 0, 0, 100, IOT_STRIP_EFFECT_ALERT);
    g_expected_state.ambience_scene = IOT_AMBIENCE_SCENE_SAFE;

    App_IOT_Scenario_Emergency();
    App_IOT_Emergency_Alert("fire", "Fire emergency triggered by garden master");

    ESP_LOGW(TAG, "Fire emergency: safety CUT_OFF, pump/atomizer OFF, lights ON, local alert dispatched");
}

void App_Garden_Fire_Reset(void) {
    ESP_LOGI(TAG, "Reset fire alarm: restore safety switch and normal ambience");
    g_expected_state.fire_status = GARDEN_FIRE_NORMAL;
    g_expected_state.safety_switch = GARDEN_SWITCH_ON;
    g_expected_state.strip_light = GARDEN_SWITCH_ON;
    g_expected_state.strip_red = 255;
    g_expected_state.strip_green = 185;
    g_expected_state.strip_blue = 110;
    g_expected_state.strip_brightness = 35;
    g_expected_state.strip_effect = IOT_STRIP_EFFECT_SOLID;
    g_expected_state.ambience_scene = IOT_AMBIENCE_SCENE_NORMAL;

    Send_IoT_Command(IOT_CMD_FIRE_RESET, 6, 1);
    Send_IoT_Command(IOT_CMD_SET_AMBIENCE_SCENE, 1, IOT_AMBIENCE_SCENE_NORMAL);
}

void App_Garden_Get_Status_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"online\":%s,"
        "\"expected\":{\"corridor_light\":%d,\"strip_light\":%d,\"veg_light\":%d,"
        "\"veg_pwm_duty\":%d,\"strip_rgb\":[%d,%d,%d],\"strip_brightness\":%d,"
        "\"strip_effect\":%d,\"ambience_scene\":\"%s\",\"fan\":%d,\"servo\":%d,\"auto_mode\":%s,"
        "\"auto_fan\":%s,\"auto_atomizer\":%s,\"auto_watering\":%s,\"auto_rain_shelter\":%s,"
        "\"water_pump\":%d,\"atomizer\":%d,\"safety_switch\":%d,\"fire_status\":%d},"
        "\"actual\":{\"corridor_light\":%d,\"strip_light\":%d,\"veg_light\":%d,"
        "\"veg_pwm_duty\":%d,\"strip_rgb\":[%d,%d,%d],\"strip_brightness\":%d,"
        "\"strip_effect\":%d,\"ambience_scene\":\"%s\",\"fan\":%d,\"servo\":%d,\"auto_mode\":%s,"
        "\"auto_fan\":%s,\"auto_atomizer\":%s,\"auto_watering\":%s,\"auto_rain_shelter\":%s,"
        "\"water_pump\":%d,\"atomizer\":%d,\"safety_switch\":%d,\"fire_status\":%d}}",
        g_garden_online ? "true" : "false",
        g_expected_state.corridor_light,
        g_expected_state.strip_light,
        g_expected_state.veg_light,
        g_expected_state.veg_pwm_duty,
        g_expected_state.strip_red,
        g_expected_state.strip_green,
        g_expected_state.strip_blue,
        g_expected_state.strip_brightness,
        g_expected_state.strip_effect,
        Garden_Scene_Name_From_Id(g_expected_state.ambience_scene),
        g_expected_state.fan,
        g_expected_state.servo,
        g_expected_state.auto_mode ? "true" : "false",
        g_expected_state.auto_fan ? "true" : "false",
        g_expected_state.auto_atomizer ? "true" : "false",
        g_expected_state.auto_watering ? "true" : "false",
        g_expected_state.auto_rain_shelter ? "true" : "false",
        g_expected_state.water_pump,
        g_expected_state.atomizer,
        g_expected_state.safety_switch,
        g_expected_state.fire_status,
        g_actual_state.corridor_light,
        g_actual_state.strip_light,
        g_actual_state.veg_light,
        g_actual_state.veg_pwm_duty,
        g_actual_state.strip_red,
        g_actual_state.strip_green,
        g_actual_state.strip_blue,
        g_actual_state.strip_brightness,
        g_actual_state.strip_effect,
        Garden_Scene_Name_From_Id(g_actual_state.ambience_scene),
        g_actual_state.fan,
        g_actual_state.servo,
        g_actual_state.auto_mode ? "true" : "false",
        g_actual_state.auto_fan ? "true" : "false",
        g_actual_state.auto_atomizer ? "true" : "false",
        g_actual_state.auto_watering ? "true" : "false",
        g_actual_state.auto_rain_shelter ? "true" : "false",
        g_actual_state.water_pump,
        g_actual_state.atomizer,
        g_actual_state.safety_switch,
        g_actual_state.fire_status);
}

void App_Garden_Get_Sensors_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;
    snprintf(buffer, buffer_size,
        "{\"soil\":%d,\"rain_percent\":%d,\"rain_adc_mv\":%d,\"rain_state\":\"%s\","
        "\"fire_status\":%d,\"help_status\":%d,"
        "\"temperature\":%.1f,\"humidity\":%.1f}",
        g_sensor_data.soil_percent,
        g_sensor_data.rain_percent,
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
            else if (strcmp(act, "rgb") == 0 && cJSON_IsObject(value)) {
                cJSON* r = cJSON_GetObjectItem(value, "red");
                cJSON* g = cJSON_GetObjectItem(value, "green");
                cJSON* b = cJSON_GetObjectItem(value, "blue");
                cJSON* br = cJSON_GetObjectItem(value, "brightness");
                cJSON* effect = cJSON_GetObjectItem(value, "effect");
                if (cJSON_IsNumber(r) && cJSON_IsNumber(g) && cJSON_IsNumber(b)) {
                    uint8_t brightness = cJSON_IsNumber(br) ? (uint8_t)br->valueint : 50;
                    uint8_t fx = cJSON_IsNumber(effect) ? (uint8_t)effect->valueint : IOT_STRIP_EFFECT_SOLID;
                    success = App_Garden_Set_Strip_RGB((uint8_t)r->valueint, (uint8_t)g->valueint, (uint8_t)b->valueint, brightness, fx);
                }
            }
        } else if (strcmp(dev_name, "veg_light") == 0 || strcmp(dev_name, "corridor_long_light") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "fan") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Fan(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Fan(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "servo") == 0 || strcmp(dev_name, "rain_shelter") == 0) {
            if (strcmp(act, "hang") == 0 || strcmp(act, "deploy") == 0 || strcmp(act, "open") == 0) {
                App_Garden_Set_Servo(GARDEN_SERVO_DEPLOY);
                success = true;
            } else if (strcmp(act, "collect") == 0 || strcmp(act, "retract") == 0 || strcmp(act, "close") == 0) {
                App_Garden_Set_Servo(GARDEN_SERVO_RETRACT);
                success = true;
            }
        } else if (strcmp(dev_name, "water_pump") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Water_Pump(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "atomizer") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_Set_Atomizer(GARDEN_SWITCH_ON); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF); success = true; }
        } else if (strcmp(dev_name, "safety_switch") == 0) {
            if (strcmp(act, "on") == 0 || strcmp(act, "normal") == 0 || strcmp(act, "connect") == 0) {
                App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);
                success = true;
            } else if (strcmp(act, "off") == 0 || strcmp(act, "cut_off") == 0 || strcmp(act, "trip") == 0) {
                App_Garden_Set_Safety_Switch(GARDEN_SWITCH_OFF);
                success = true;
            }
        } else if (strcmp(dev_name, "auto_mode") == 0) {
            if (value != NULL) {
                App_Garden_Set_Auto_Mode(cJSON_IsTrue(value));
                success = true;
            }
        } else if (strcmp(dev_name, "device_auto") == 0 || strcmp(dev_name, "auto_device") == 0) {
            if (cJSON_IsObject(value)) {
                cJSON* target = cJSON_GetObjectItem(value, "target");
                cJSON* enable = cJSON_GetObjectItem(value, "enable");
                if (cJSON_IsString(target) && enable != NULL) {
                    success = App_Garden_Set_Device_Auto(target->valuestring, cJSON_IsTrue(enable));
                }
            }
        } else if (strcmp(dev_name, "all_lights") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_All_Lights_On(); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_All_Lights_Off(); success = true; }
        } else if (strcmp(dev_name, "all_devices") == 0) {
            if (strcmp(act, "on") == 0) { App_Garden_All_Devices_On(); success = true; }
            else if (strcmp(act, "off") == 0) { App_Garden_All_Devices_Off(); success = true; }
        } else if (strcmp(dev_name, "ambience") == 0 || strcmp(dev_name, "ambience_scene") == 0) {
            if (strcmp(act, "scene") == 0 && cJSON_IsString(value)) {
                success = App_Garden_Set_Ambience_Scene(value->valuestring);
            } else if (strcmp(act, "calibrate") == 0) {
                App_Garden_Set_Strip_RGB(255, 0, 0, 60, IOT_STRIP_EFFECT_SOLID);
                App_Garden_Set_Strip_RGB(0, 255, 0, 60, IOT_STRIP_EFFECT_SOLID);
                App_Garden_Set_Strip_RGB(0, 0, 255, 60, IOT_STRIP_EFFECT_SOLID);
                success = App_Garden_Set_Ambience_Scene("normal");
            }
        } else if (strcmp(dev_name, "emergency") == 0) {
            if (strcmp(act, "fire") == 0) { App_Garden_Fire_Emergency_Response(); success = true; }
            else if (strcmp(act, "reset") == 0 || strcmp(act, "fire_reset") == 0) { App_Garden_Fire_Reset(); success = true; }
        }
    }

    cJSON_Delete(root);
    return success;
}
