#include "mqtt_receive.h"
#include "iot_control_task.h"
#include "mqtt_iot_protocol.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_netif.h"

static const char* TAG = "MQTT_RECV";

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static char s_mac_str[13] = {0};

static emergency_event_cb_t s_emergency_cb = NULL;

#define DEVICE_NAME "智慧庭院"
#define TOTAL_GPIO_COUNT 10

#define MQTT_BROKER_URI     "mqtt://8.134.167.240"
#define MQTT_BROKER_PORT    1883

static void Send_Response(uint8_t cmd, uint8_t gpio_index, uint8_t value) {
    if (s_mqtt_client == NULL) return;

    iot_command_packet_t resp = {
        .command = cmd,
        .device_id = 0,
        .gpio_index = gpio_index,
        .value = value,
        .reserved = {0}
    };

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_RESP_PREFIX "%s", s_mac_str);
    esp_mqtt_client_publish(s_mqtt_client, topic, (const char*)&resp, sizeof(resp), 0, 0);
    ESP_LOGD(TAG, "Response sent: cmd=%d, gpio=%d, value=%d", cmd, gpio_index, value);
}

static void Send_Announce(void) {
    if (s_mqtt_client == NULL) return;

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
    ESP_LOGI(TAG, "Command: cmd=%d, gpio=%d, value=%d",
            cmd->command, cmd->gpio_index, cmd->value);

    switch (cmd->command) {
    case IOT_CMD_SET_GPIO:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < LIGHT_COUNT) {
            g_device_flags.light[cmd->gpio_index] = (cmd->value == 1) ? ON : OFF;
            ESP_LOGI(TAG, "Light[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        } else if (cmd->gpio_index < LIGHT_COUNT + RELAY_COUNT) {
            g_device_flags.relay[cmd->gpio_index - LIGHT_COUNT] = (cmd->value == 1) ? ON : OFF;
            ESP_LOGI(TAG, "Relay[%d] = %s", cmd->gpio_index - LIGHT_COUNT, cmd->value ? "ON" : "OFF");
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_RESPONSE, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_LIGHT:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < LIGHT_COUNT) {
            g_device_flags.light[cmd->gpio_index] = (cmd->value == 1) ? ON : OFF;
            ESP_LOGI(TAG, "Light[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_RESPONSE, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_RELAY:
        DEVICE_FLAGS_LOCK();
        if (cmd->gpio_index < RELAY_COUNT) {
            g_device_flags.relay[cmd->gpio_index] = (cmd->value == 1) ? ON : OFF;
            ESP_LOGI(TAG, "Relay[%d] = %s", cmd->gpio_index, cmd->value ? "ON" : "OFF");
        }
        DEVICE_FLAGS_UNLOCK();
        Send_Response(IOT_CMD_RESPONSE, cmd->gpio_index, cmd->value);
        break;

    case IOT_CMD_SET_SERVO:
        if (cmd->gpio_index >= 6 && cmd->gpio_index < 6 + SERVO_COUNT) {
            uint8_t local_idx = cmd->gpio_index - 6;
            int16_t angle = (int16_t)cmd->value;
            if (cmd->reserved[0] != 0) {
                angle = (int16_t)((cmd->reserved[0] << 8) | cmd->value);
            }
            if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
            if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
            DEVICE_FLAGS_LOCK();
            g_device_flags.servo[local_idx] = angle;
            DEVICE_FLAGS_UNLOCK();
            ESP_LOGI(TAG, "Servo[%d] = %d deg", local_idx, (int)angle);
            Send_Response(IOT_CMD_RESPONSE, cmd->gpio_index, (uint8_t)angle);
        }
        break;

    case IOT_CMD_SET_AUTO_MODE:
        DEVICE_FLAGS_LOCK();
        g_device_flags.auto_mode = (cmd->value == 1);
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Auto mode = %s", cmd->value ? "ON" : "OFF");
        Send_Response(IOT_CMD_RESPONSE, 0, cmd->value);
        break;

    case IOT_CMD_FIRE_RESET:
        DEVICE_FLAGS_LOCK();
        g_device_flags.fire_status = FIRE_NORMAL;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "Fire status reset to NORMAL");
        Send_Response(IOT_CMD_RESPONSE, 0, 0);
        break;

    case IOT_CMD_DISCOVER:
        ESP_LOGI(TAG, "Master discovered via MQTT");
        Send_Announce();
        break;

    case IOT_CMD_BROADCAST_ALL_OFF:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = OFF;
        for (int i = 0; i < RELAY_COUNT; i++) g_device_flags.relay[i] = OFF;
        for (int i = 0; i < SERVO_COUNT; i++) g_device_flags.servo[i] = SERVO_MIN_ANGLE;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All devices OFF");
        break;

    case IOT_CMD_BROADCAST_ALL_ON:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = ON;
        for (int i = 0; i < RELAY_COUNT; i++) g_device_flags.relay[i] = ON;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All lights and relays ON");
        break;

    case IOT_CMD_BROADCAST_LIGHTS_OFF:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = OFF;
        DEVICE_FLAGS_UNLOCK();
        ESP_LOGI(TAG, "All lights OFF");
        break;

    case IOT_CMD_BROADCAST_LIGHTS_ON:
        DEVICE_FLAGS_LOCK();
        for (int i = 0; i < LIGHT_COUNT; i++) g_device_flags.light[i] = ON;
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
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_CMD_BROADCAST, 1);
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_CMD_PREFIX DEVICE_ID_GARDEN, 1);
            Send_Announce();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
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
                    const iot_command_packet_t* cmd = (const iot_command_packet_t*)event->data;
                    Process_Command(cmd);
                }
            } else if (event->data_len > 0 && event->data_len < (int)sizeof(iot_command_packet_t)) {
                ESP_LOGD(TAG, "Short packet on non-cmd topic: %d bytes", event->data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            break;

        default:
            break;
    }
}

void MQTT_Receive_Init(void) {
    ESP_LOGI(TAG, "Initializing MQTT...");

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    mac_to_string(mac, s_mac_str, sizeof(s_mac_str));

    char client_id[24] = {0};
    snprintf(client_id, sizeof(client_id), "xiaozhi_slave_%s", s_mac_str);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .broker.address.port = MQTT_BROKER_PORT,
        .credentials.client_id = client_id,
        .session.keepalive = 30,
        .network.disable_auto_reconnect = false,
        .network.reconnect_timeout_ms = 5000,
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to init MQTT client");
        return;
    }

    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_mqtt_client);

    ESP_LOGI(TAG, "MQTT initialized (client_id: %s, device: %s)", client_id, DEVICE_NAME);
}

void MQTT_Register_Emergency_Callback(emergency_event_cb_t cb) {
    s_emergency_cb = cb;
}

void MQTT_Publish_Emergency(emergency_type_t type, const char* detail) {
    if (s_mqtt_client == NULL) {
        ESP_LOGW(TAG, "MQTT client not ready, cannot publish emergency");
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

esp_mqtt_client_handle_t MQTT_Get_Client(void) {
    return s_mqtt_client;
}

const char* MQTT_Get_MAC_Str(void) {
    return s_mac_str;
}
