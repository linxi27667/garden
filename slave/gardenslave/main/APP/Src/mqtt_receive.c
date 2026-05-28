#include "mqtt_receive.h"
#include "iot_control_task.h"
#include "sensor_adc_task.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static const char* TAG = "MQTT_RECV";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;
static char s_mac_str[13] = {0};

static emergency_event_cb_t s_emergency_cb = NULL;

#define DEVICE_NAME "智慧庭院"
#define TOTAL_GPIO_COUNT 8

#define MQTT_BROKER_URI     "mqtt://8.134.167.240"
#define MQTT_BROKER_PORT    1883
#define SERVO_GPIO_INDEX_BASE (LIGHT_COUNT + RELAY_COUNT)

static bool MQTT_Can_Publish(const char* reason) {
    if (s_mqtt_client == NULL || !s_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT not connected, skip %s", reason ? reason : "publish");
        return false;
    }
    return true;
}

static bool Get_Device_Value_By_Global_Index(uint8_t gpio_index, uint8_t* value) {
    if (value == NULL) {
        return false;
    }

    if (!Iot_Device_Flags_Lock(__func__)) {
        return false;
    }
    if (gpio_index < LIGHT_COUNT) {
        *value = (uint8_t)g_device_flags.light[gpio_index];
        Iot_Device_Flags_Unlock(__func__);
        return true;
    }

    if (gpio_index < LIGHT_COUNT + RELAY_COUNT) {
        uint8_t relay_idx = gpio_index - LIGHT_COUNT;
        *value = (uint8_t)g_device_flags.relay[relay_idx];
        Iot_Device_Flags_Unlock(__func__);
        return true;
    }

    if (gpio_index >= SERVO_GPIO_INDEX_BASE &&
        gpio_index < SERVO_GPIO_INDEX_BASE + SERVO_COUNT) {
        uint8_t servo_idx = gpio_index - SERVO_GPIO_INDEX_BASE;
        *value = (uint8_t)g_device_flags.servo[servo_idx];
        Iot_Device_Flags_Unlock(__func__);
        return true;
    }

    Iot_Device_Flags_Unlock(__func__);
    return false;
}

static void Send_Response(uint8_t original_cmd, uint8_t gpio_index, uint8_t value) {
    if (!MQTT_Can_Publish("response")) return;

    iot_command_packet_t resp = {
        .command = IOT_CMD_RESPONSE,
        .device_id = 10,
        .gpio_index = gpio_index,
        .value = value,
        .reserved = {original_cmd, 0, 0, 0}
    };

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_RESP_PREFIX "%s", s_mac_str);
    esp_mqtt_client_publish(s_mqtt_client, topic, (const char*)&resp, sizeof(resp), 0, 0);
    ESP_LOGD(TAG, "Response sent: original=%d, gpio=%d, value=%d", original_cmd, gpio_index, value);
}

static const char* g_scene_names[] = {
    "自定义", "月白风清", "清风徐来", "闲庭信步",
    "烟雨朦胧", "安全警戒", "风和日丽", "万籁俱寂"
};

static void Apply_Ambience_Scene_Locked(uint8_t scene_id) {
    g_device_flags.ambience_scene = scene_id;

    const char* name = (scene_id < sizeof(g_scene_names) / sizeof(g_scene_names[0]))
                       ? g_scene_names[scene_id] : "未知";
    ESP_LOGI(TAG, "=== 意境切换: %s (id=%d) ===", name, (int)scene_id);
    MQTT_Publish_Scene(scene_id);

    switch (scene_id) {
    case IOT_AMBIENCE_SCENE_NORMAL:  /* 风和日丽 */
        g_device_flags.light[0] = SWITCH_ON;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_ON;
        g_device_flags.relay[0] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_OFF;
        g_device_flags.strip_red = 255;
        g_device_flags.strip_green = 195;
        g_device_flags.strip_blue = 80;
        g_device_flags.strip_brightness = 40;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_SOLID;
        break;
    case IOT_AMBIENCE_SCENE_NIGHT:   /* 月白风清 */
        g_device_flags.light[0] = SWITCH_ON;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_ON;
        g_device_flags.relay[0] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_OFF;
        g_device_flags.strip_red = 190;
        g_device_flags.strip_green = 200;
        g_device_flags.strip_blue = 235;
        g_device_flags.strip_brightness = 15;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_SOLID;
        break;
    case IOT_AMBIENCE_SCENE_HOT:     /* 清风徐来 */
        g_device_flags.light[0] = SWITCH_OFF;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_OFF;
        g_device_flags.relay[0] = SWITCH_ON;
        g_device_flags.relay[2] = SWITCH_ON;
        g_device_flags.strip_red = 70;
        g_device_flags.strip_green = 185;
        g_device_flags.strip_blue = 250;
        g_device_flags.strip_brightness = 45;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_BREATHE;
        break;
    case IOT_AMBIENCE_SCENE_RELAX:   /* 闲庭信步 */
        g_device_flags.light[0] = SWITCH_ON;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_OFF;
        g_device_flags.relay[0] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_ON;
        g_device_flags.strip_red = 65;
        g_device_flags.strip_green = 210;
        g_device_flags.strip_blue = 95;
        g_device_flags.strip_brightness = 28;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_BREATHE;
        break;
    case IOT_AMBIENCE_SCENE_RAIN:    /* 烟雨朦胧 */
        g_device_flags.light[0] = SWITCH_ON;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_ON;
        g_device_flags.relay[0] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_OFF;
        g_device_flags.strip_red = 50;
        g_device_flags.strip_green = 85;
        g_device_flags.strip_blue = 200;
        g_device_flags.strip_brightness = 25;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_BREATHE;
        break;
    case IOT_AMBIENCE_SCENE_SAFE:    /* 安全警戒 */
        g_device_flags.light[0] = SWITCH_ON;
        g_device_flags.light[1] = SWITCH_ON;
        g_device_flags.light[2] = SWITCH_ON;
        g_device_flags.strip_red = 255;
        g_device_flags.strip_green = 255;
        g_device_flags.strip_blue = 255;
        g_device_flags.strip_brightness = 100;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_ALERT;
        g_device_flags.relay[1] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_OFF;
        g_device_flags.relay[3] = SWITCH_ON;
        g_device_flags.auto_mode = false;
        break;
    case IOT_AMBIENCE_SCENE_ECO:     /* 万籁俱寂 */
        g_device_flags.light[0] = SWITCH_OFF;
        g_device_flags.light[1] = SWITCH_OFF;
        g_device_flags.light[2] = SWITCH_OFF;
        g_device_flags.relay[0] = SWITCH_OFF;
        g_device_flags.relay[2] = SWITCH_OFF;
        g_device_flags.strip_red = 0;
        g_device_flags.strip_green = 0;
        g_device_flags.strip_blue = 0;
        g_device_flags.strip_brightness = 0;
        g_device_flags.strip_effect = IOT_STRIP_EFFECT_SOLID;
        break;
    case IOT_AMBIENCE_SCENE_CUSTOM:
    default:
        g_device_flags.ambience_scene = IOT_AMBIENCE_SCENE_CUSTOM;
        break;
    }
}

static void Send_Announce(void) {
    if (!MQTT_Can_Publish("announce")) return;

    iot_announce_v2_packet_t resp = {
        .command = IOT_CMD_ANNOUNCE_V2,
        .device_id = 10,
        .gpio_index = TOTAL_GPIO_COUNT,
        .value = 1,
        .device_name = DEVICE_NAME
    };

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_ANNOUNCE_PREFIX "%s", s_mac_str);
    esp_mqtt_client_publish(s_mqtt_client, topic, (const char*)&resp, sizeof(resp), 1, 0);
    ESP_LOGI(TAG, "Announce V2 sent: %s (%d GPIOs)", DEVICE_NAME, TOTAL_GPIO_COUNT);
}

static void Process_Command(const iot_command_packet_t* cmd) {
    ESP_LOGD(TAG, "Command: cmd=%d, gpio=%d, value=%d",
            cmd->command, cmd->gpio_index, cmd->value);

    switch (cmd->command) {
    case IOT_CMD_SET_GPIO:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < LIGHT_COUNT) {
            g_device_flags.light[cmd->gpio_index] = (cmd->value == 1) ? SWITCH_ON : SWITCH_OFF;
            ESP_LOGI(TAG, "Light[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        } else if (cmd->gpio_index < LIGHT_COUNT + RELAY_COUNT) {
            uint8_t relay_idx = cmd->gpio_index - LIGHT_COUNT;
            g_device_flags.relay[relay_idx] = (cmd->value == 1) ? SWITCH_ON : SWITCH_OFF;
            Sensor_ADC_Notify_Relay_Manual(relay_idx);
            ESP_LOGI(TAG, "Relay[%d] = %s", relay_idx, cmd->value ? "ON" : "OFF");
        } else if (cmd->gpio_index >= SERVO_GPIO_INDEX_BASE &&
                   cmd->gpio_index < SERVO_GPIO_INDEX_BASE + SERVO_COUNT) {
            uint8_t servo_idx = cmd->gpio_index - SERVO_GPIO_INDEX_BASE;
            g_device_flags.servo[servo_idx] = cmd->value ? SERVO_MAX_ANGLE : SERVO_MIN_ANGLE;
            Sensor_ADC_Notify_Servo_Manual(servo_idx);
            ESP_LOGI(TAG, "Servo[%d] = %d deg", servo_idx, g_device_flags.servo[servo_idx]);
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_SET_GPIO, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_LIGHT:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < LIGHT_COUNT) {
            g_device_flags.light[cmd->gpio_index] = (cmd->value == 1) ? SWITCH_ON : SWITCH_OFF;
            ESP_LOGI(TAG, "Light[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_SET_LIGHT, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_RELAY:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < RELAY_COUNT) {
            g_device_flags.relay[cmd->gpio_index] = (cmd->value == 1) ? SWITCH_ON : SWITCH_OFF;
            Sensor_ADC_Notify_Relay_Manual(cmd->gpio_index);
            ESP_LOGI(TAG, "Relay[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_SET_RELAY, LIGHT_COUNT + cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_STRIP_RGB:
        DEVICE_FLAGS_LOCK();
        g_device_flags.light[1] = (cmd->value > 0) ? SWITCH_ON : SWITCH_OFF;
        g_device_flags.strip_brightness = cmd->value > 100 ? 100 : cmd->value;
        g_device_flags.strip_red = cmd->reserved[0];
        g_device_flags.strip_green = cmd->reserved[1];
        g_device_flags.strip_blue = cmd->reserved[2];
        g_device_flags.strip_effect = cmd->reserved[3];
        g_device_flags.ambience_scene = IOT_AMBIENCE_SCENE_CUSTOM;
        DEVICE_FLAGS_UNLOCK();
        MQTT_Publish_Scene(IOT_AMBIENCE_SCENE_CUSTOM);
        ESP_LOGI(TAG, "Strip RGB: r=%d g=%d b=%d brightness=%d effect=%d",
                 cmd->reserved[0], cmd->reserved[1], cmd->reserved[2], cmd->value, cmd->reserved[3]);
        Send_Response(IOT_CMD_SET_STRIP_RGB, 1, cmd->value);
        break;

    case IOT_CMD_SET_AMBIENCE_SCENE:
        DEVICE_FLAGS_LOCK();
        Apply_Ambience_Scene_Locked(cmd->value);
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Ambience scene = %d", cmd->value);
        Send_Response(IOT_CMD_SET_AMBIENCE_SCENE, 1, cmd->value);
        break;

    case IOT_CMD_SET_SERVO:
        if (cmd->gpio_index >= SERVO_GPIO_INDEX_BASE &&
            cmd->gpio_index < SERVO_GPIO_INDEX_BASE + SERVO_COUNT) {
            uint8_t local_idx = cmd->gpio_index - SERVO_GPIO_INDEX_BASE;
            int16_t angle = (int16_t)cmd->value;
            if (cmd->reserved[0] != 0) {
                angle = (int16_t)((cmd->reserved[0] << 8) | cmd->value);
            }
            if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
            if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
            DEVICE_FLAGS_LOCK();
            g_device_flags.servo[local_idx] = angle;
            Sensor_ADC_Notify_Servo_Manual(local_idx);
            DEVICE_FLAGS_UNLOCK();
            ESP_LOGI(TAG, "Servo[%d] = %d deg", local_idx, (int)angle);
            Send_Response(IOT_CMD_SET_SERVO, cmd->gpio_index, (uint8_t)angle);
        }
        break;

    case IOT_CMD_SET_AUTO_MODE:
        DEVICE_FLAGS_LOCK();
        g_device_flags.auto_mode = (cmd->value == 1);
        if (g_device_flags.auto_mode) {
            g_device_flags.auto_fan = true;
            g_device_flags.auto_atomizer = true;
            g_device_flags.auto_watering = true;
            g_device_flags.auto_rain_shelter = true;
        }
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Auto mode = %s", cmd->value ? "ON" : "OFF");
        Send_Response(IOT_CMD_SET_AUTO_MODE, 0, cmd->value);
        break;

    case IOT_CMD_SET_DEVICE_AUTO:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index == IOT_AUTO_DEVICE_FAN) {
            g_device_flags.auto_fan = (cmd->value == 1);
        } else if (cmd->gpio_index == IOT_AUTO_DEVICE_ATOMIZER) {
            g_device_flags.auto_atomizer = (cmd->value == 1);
        } else if (cmd->gpio_index == IOT_AUTO_DEVICE_WATERING) {
            g_device_flags.auto_watering = (cmd->value == 1);
        } else if (cmd->gpio_index == IOT_AUTO_DEVICE_RAIN_SHELTER) {
            g_device_flags.auto_rain_shelter = (cmd->value == 1);
        }
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Device auto[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        Send_Response(IOT_CMD_SET_DEVICE_AUTO, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_FIRE_RESET:
        DEVICE_FLAGS_LOCK();
        g_device_flags.fire_status = FIRE_NORMAL;
        g_device_flags.relay[3] = SWITCH_ON;
        Apply_Ambience_Scene_Locked(IOT_AMBIENCE_SCENE_NORMAL);
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Fire status reset to NORMAL, safety switch ON, ambience restored");
        Send_Response(IOT_CMD_FIRE_RESET, LIGHT_COUNT + 3, 1);
        break;

    case IOT_CMD_GET_GPIO: {
        uint8_t value = 0;
        if (Get_Device_Value_By_Global_Index(cmd->gpio_index, &value)) {
            Send_Response(IOT_CMD_GET_GPIO, cmd->gpio_index, value);
        } else {
            ESP_LOGW(TAG, "GET_GPIO invalid index: %d", cmd->gpio_index);
        }
        break;
    }

    case IOT_CMD_GET_ALL_GPIO:
        for (uint8_t i = 0; i < TOTAL_GPIO_COUNT; i++) {
            uint8_t value = 0;
            if (Get_Device_Value_By_Global_Index(i, &value)) {
                Send_Response(IOT_CMD_GET_ALL_GPIO, i, value);
            }
        }
        break;

    case IOT_CMD_DISCOVER:
        ESP_LOGD(TAG, "Master discovered via MQTT");
        Send_Announce();
        break;

    case IOT_CMD_BROADCAST_ALL_OFF:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = SWITCH_OFF;
        for (int i = 0; i < RELAY_COUNT; i++) g_device_flags.relay[i] = SWITCH_OFF;
        g_device_flags.relay[3] = SWITCH_ON;
        for (int i = 0; i < SERVO_COUNT; i++) g_device_flags.servo[i] = SERVO_MIN_ANGLE;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All user devices OFF, safety switch kept ON");
        break;

    case IOT_CMD_BROADCAST_ALL_ON:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = SWITCH_ON;
        for (int i = 0; i < RELAY_COUNT; i++) g_device_flags.relay[i] = SWITCH_ON;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All lights and relays ON");
        break;

    case IOT_CMD_BROADCAST_LIGHTS_OFF:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = SWITCH_OFF;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All lights OFF");
        break;

    case IOT_CMD_BROADCAST_LIGHTS_ON:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = SWITCH_ON;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All lights ON");
        break;

    case IOT_CMD_EMERGENCY:
        ESP_LOGW(TAG, "EMERGENCY command received from master!");
        Iot_Fire_Emergency_Response();
        if (s_emergency_cb != NULL) {
            s_emergency_cb(EMERGENCY_FIRE, "Fire emergency triggered from master");
        }
        break;

    default:
        ESP_LOGW(TAG, "Unknown command: %d", cmd->command);
        break;
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            s_mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT connected: broker=%s:%d, client=%s",
                     MQTT_BROKER_URI, MQTT_BROKER_PORT, s_mac_str);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_CMD_BROADCAST, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_CMD_PREFIX DEVICE_ID_GARDEN, 1);
            Send_Announce();
            break;

        case MQTT_EVENT_DISCONNECTED:
            s_mqtt_connected = false;
            ESP_LOGW(TAG, "MQTT disconnected, auto reconnect is enabled (%d ms)",
                     5000);
            break;

        case MQTT_EVENT_DATA:
            if (event->topic_len > 0 && event->data_len >= (int)sizeof(iot_command_packet_t)) {
                char topic_buf[128] = {0};
                if (event->topic_len < (int)sizeof(topic_buf)) {
                    memcpy(topic_buf, event->topic, event->topic_len);
                    topic_buf[event->topic_len] = '\0';
                }

                bool is_cmd_topic = (strstr(topic_buf, MQTT_TOPIC_CMD_PREFIX) != NULL ||
                                     strstr(topic_buf, MQTT_TOPIC_CMD_BROADCAST) != NULL);
                if (is_cmd_topic) {
                    iot_command_packet_t cmd_buf;
                    memcpy(&cmd_buf, event->data, sizeof(cmd_buf));
                    Process_Command(&cmd_buf);
                }
            } else if (event->data_len > 0 && event->data_len < (int)sizeof(iot_command_packet_t)) {
                ESP_LOGD(TAG, "Short packet on non-cmd topic: %d bytes", event->data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            s_mqtt_connected = false;
            if (event->error_handle) {
                ESP_LOGE(TAG,
                         "MQTT error: type=%d, esp_tls=%s, tls_stack=0x%x, sock_errno=%d (%s), reconnect=%s",
                         event->error_handle->error_type,
                         esp_err_to_name(event->error_handle->esp_tls_last_esp_err),
                         event->error_handle->esp_tls_stack_err,
                         event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno),
                         "enabled");
            } else {
                ESP_LOGE(TAG, "MQTT error: no error handle, reconnect=enabled");
            }
            break;

        default:
            break;
    }
}

void MQTT_Receive_Init(void) {
    ESP_LOGI(TAG, "Initializing MQTT...");

    uint8_t mac[6];
    esp_err_t err = esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read MAC: %d", err);
        return;
    }
    mac_to_string(mac, s_mac_str, sizeof(s_mac_str));
    ESP_LOGI(TAG, "MAC: %s", s_mac_str);

    char client_id[32] = {0};
    snprintf(client_id, sizeof(client_id), "xiaozhi_slave_%s", s_mac_str);
    ESP_LOGI(TAG, "Client ID: %s", client_id);

    size_t free_heap = esp_get_free_heap_size();
    ESP_LOGI(TAG, "Free heap: %d bytes", (int)free_heap);
    if (free_heap < 8192) {
        ESP_LOGE(TAG, "Insufficient heap memory!");
        return;
    }

    esp_mqtt_client_config_t* mqtt_cfg = calloc(1, sizeof(esp_mqtt_client_config_t));
    if (mqtt_cfg == NULL) {
        ESP_LOGE(TAG, "Failed to allocate MQTT config");
        return;
    }

    mqtt_cfg->broker.address.uri = MQTT_BROKER_URI;
    mqtt_cfg->broker.address.port = MQTT_BROKER_PORT;
    mqtt_cfg->credentials.client_id = client_id;
    mqtt_cfg->session.keepalive = 30;
    mqtt_cfg->network.disable_auto_reconnect = false;
    mqtt_cfg->network.reconnect_timeout_ms = 5000;

    ESP_LOGI(TAG, "MQTT config ready");

    s_mqtt_client = esp_mqtt_client_init(mqtt_cfg);
    free(mqtt_cfg);

    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return;
    }
    ESP_LOGI(TAG, "MQTT client init OK");

    err = esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register event: %d", err);
        return;
    }
    ESP_LOGI(TAG, "MQTT event registered");

    err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT: %d", err);
        return;
    }
    ESP_LOGI(TAG, "MQTT started");

    ESP_LOGI(TAG, "MQTT initialized (client_id: %s, device: %s)", client_id, DEVICE_NAME);
}

void MQTT_Register_Emergency_Callback(emergency_event_cb_t cb) {
    s_emergency_cb = cb;
}

void MQTT_Publish_Emergency(emergency_type_t type, const char* detail) {
    if (!MQTT_Can_Publish("emergency")) {
        return;
    }

    char payload[256];
    int payload_len = snprintf(payload, sizeof(payload),
        "{\"type\":\"emergency\",\"device_id\":\"%s\",\"emergency_type\":%d,\"detail\":\"%s\"}",
        s_mac_str, (int)type, detail ? detail : "");

    if (payload_len < 0 || payload_len >= (int)sizeof(payload)) {
        ESP_LOGE(TAG, "Emergency payload too long or encoding error");
        return;
    }

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_EMERGENCY_PREFIX "%s", s_mac_str);

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, payload, payload_len, 1, 0);
    ESP_LOGI(TAG, "Emergency published: type=%d, topic=%s, msg_id=%d", type, topic, msg_id);
}

void MQTT_Publish_Scene(uint8_t scene_id) {
    if (!MQTT_Can_Publish("scene")) return;

    const char* name = (scene_id < sizeof(g_scene_names) / sizeof(g_scene_names[0]))
                       ? g_scene_names[scene_id] : "未知";

    char topic[64], payload[128];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_STATUS_PREFIX "%s", s_mac_str);
    int len = snprintf(payload, sizeof(payload),
        "{\"type\":\"scene\",\"scene_id\":%d,\"scene_name\":\"%s\"}", scene_id, name);

    if (len > 0 && len < (int)sizeof(payload)) {
        esp_mqtt_client_publish(s_mqtt_client, topic, payload, len, 0, 0);
        ESP_LOGI(TAG, "Scene published: %s (id=%d)", name, (int)scene_id);
    }
}

esp_mqtt_client_handle_t MQTT_Get_Client(void) {
    return s_mqtt_client;
}

const char* MQTT_Get_MAC_Str(void) {
    return s_mac_str;
}

bool MQTT_Is_Connected(void) {
    return s_mqtt_connected;
}

void MQTT_Publish_Energy(const energy_accum_t* data, const bool on_state[ENERGY_DEV_COUNT]) {
    if (!MQTT_Can_Publish("energy")) return;

    float total = 0;
    for (int i = 0; i < ENERGY_DEV_COUNT; i++) total += data->kwh[i];

    char buf[512];
    int len = snprintf(buf, sizeof(buf),
        "{"
          "\"devices\":{"
            "\"corridor_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"strip_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"veg_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"fan\":{\"kwh\":%.3f,\"on\":%s},"
            "\"water_pump\":{\"kwh\":%.3f,\"on\":%s},"
            "\"atomizer\":{\"kwh\":%.3f,\"on\":%s}"
          "},"
          "\"total_kwh\":%.3f"
        "}",
        data->kwh[0], on_state[0] ? "true" : "false",
        data->kwh[1], on_state[1] ? "true" : "false",
        data->kwh[2], on_state[2] ? "true" : "false",
        data->kwh[3], on_state[3] ? "true" : "false",
        data->kwh[4], on_state[4] ? "true" : "false",
        data->kwh[5], on_state[5] ? "true" : "false",
        total
    );

    if (len < 0 || len >= (int)sizeof(buf)) {
        ESP_LOGE(TAG, "Energy payload too long");
        return;
    }

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_ENERGY_PREFIX "%s", s_mac_str);
    esp_mqtt_client_publish(s_mqtt_client, topic, buf, len, 1, 1);
    ESP_LOGI(TAG, "Energy published: total=%.3f kWh", total);
}
