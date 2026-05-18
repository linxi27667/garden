/**
 * @file    bsp_iot_controller.c
 * @brief   BSP IOT 控制器实现
 *
 * 职责：
 * - 管理远程设备列表
 * - 处理 MQTT 通信协议
 * - 提供设备查询和控制接口
 */
#include "bsp_iot_controller.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

static const char* TAG = "BSP_IOT";

#define GARDEN_DEVICE_ID            10
#define GARDEN_LIGHT_COUNT          3
#define GARDEN_RELAY_COUNT          4
#define GARDEN_SERVO_GPIO_BASE      (GARDEN_LIGHT_COUNT + GARDEN_RELAY_COUNT)

typedef struct {
    const char* alias;
    const char* real_name;
} device_name_alias_t;

static const device_name_alias_t g_device_aliases[] = {
    {"一楼大门", "一楼设备"},
    {"一楼设备", "一楼设备"},
    {"一楼", "一楼设备"},
    {"大厅灯", "一楼设备"},
    {"1F_Device", "一楼设备"},  // simulator alias
    {"二楼设备", "二楼设备"},
    {"二楼", "二楼设备"},
    {"主卧", "二楼设备"},
    {"主卧灯", "二楼设备"},
    {"次卧", "二楼设备"},
    {"客厅", "二楼设备"},
    {"二楼风扇", "二楼设备"},
    {"风扇", "二楼设备"},
    {"2F_Device", "二楼设备"},  // simulator alias
    {"三楼", "三楼设备"},
    {"三楼设备", "三楼设备"},
    {"阳台灯", "三楼设备"},
    {"次卧灯", "二楼设备"},
    {"客厅灯", "二楼设备"},
    {"厕所灯", "二楼设备"},
    {"3F_Device", "三楼设备"},  // simulator alias
    {"智慧庭院", "智慧庭院"},
    {"庭院", "智慧庭院"},
    {"花园", "智慧庭院"},
    {NULL, NULL}
};

static const char* g_garden_gpio_names[] = {
    "courtyard_west_light",
    "strip_light",
    "corridor_long_light",
    "fan",
    "water_pump",
    "atomizer",
    "safety_switch",
    "clothes_servo"
};

static const char* IOT_Resolve_Alias(const char* name) {
    if (name == NULL) return NULL;
    for (int i = 0; g_device_aliases[i].alias != NULL; i++) {
        if (strcmp(name, g_device_aliases[i].alias) == 0) {
            return g_device_aliases[i].real_name;
        }
    }
    return name;
}

uint8_t IOT_Get_Device_Id_By_Name(const char* name) {
    if (name == NULL) return 0;
    if (strstr(name, "一楼") != NULL || strstr(name, "1F") != NULL) return 1;
    if (strstr(name, "二楼") != NULL || strstr(name, "2F") != NULL) return 2;
    if (strstr(name, "三楼") != NULL || strstr(name, "3F") != NULL) return 3;
    if (strstr(name, "智慧庭院") != NULL || strstr(name, "庭院") != NULL || strstr(name, "花园") != NULL) return 10;
    return 0;
}

static bool IOT_Is_Garden_Device(const iot_remote_device_t* device) {
    if (device == NULL) {
        return false;
    }

    return device->device_id == GARDEN_DEVICE_ID ||
           strstr(device->device_name, "智慧庭院") != NULL ||
           strstr(device->device_name, "庭院") != NULL ||
           strstr(device->device_name, "花园") != NULL;
}

static void IOT_Assign_Garden_GPIO_Names(iot_remote_device_t* device) {
    if (device == NULL || !IOT_Is_Garden_Device(device)) {
        return;
    }

    int count = device->gpio_count;
    int garden_name_count = sizeof(g_garden_gpio_names) / sizeof(g_garden_gpio_names[0]);
    if (count > garden_name_count) {
        count = garden_name_count;
    }

    for (int i = 0; i < count; i++) {
        strncpy(device->gpios[i].name, g_garden_gpio_names[i], sizeof(device->gpios[i].name) - 1);
        device->gpios[i].name[sizeof(device->gpios[i].name) - 1] = '\0';
        device->gpios[i].mode = IOT_GPIO_MODE_OUTPUT;
    }
}

static int IOT_Get_Servo_GPIO_Base(const iot_remote_device_t* device) {
    if (IOT_Is_Garden_Device(device)) {
        return GARDEN_SERVO_GPIO_BASE;
    }

    return 6;
}

int IOT_Add_Remote_Device(iot_controller_t* dev, const char* name, const uint8_t* mac, int gpio_count) {
    if (dev == NULL || dev->remote_device_count >= IOT_MAX_REMOTE_DEVICES) {
        return -1;
    }

    int index = dev->remote_device_count;

    if (name != NULL) {
        strncpy(dev->remote_devices[index].device_name, name, IOT_DEVICE_NAME_LEN - 1);
        dev->remote_devices[index].device_name[IOT_DEVICE_NAME_LEN - 1] = '\0';
    }

    dev->remote_devices[index].device_id = (name != NULL) ?
        IOT_Get_Device_Id_By_Name(name) : (index + 1);

    if (mac != NULL) {
        memcpy(dev->remote_devices[index].mac_addr, mac, 6);
    }

    dev->remote_devices[index].gpio_count = gpio_count > IOT_MAX_REMOTE_GPIOS ? IOT_MAX_REMOTE_GPIOS : gpio_count;
    dev->remote_devices[index].online = true;
    dev->remote_devices[index].last_heartbeat_ms = (uint32_t)(esp_timer_get_time() / 1000);
    memset(dev->remote_devices[index].gpio_states, 0, IOT_MAX_REMOTE_GPIOS);

    const char* default_names[4] = {"led1", "led2", "led3", "led4"};
    for (int i = 0; i < dev->remote_devices[index].gpio_count; i++) {
        if (i < 4) {
            strncpy(dev->remote_devices[index].gpios[i].name, default_names[i], 15);
        } else {
            snprintf(dev->remote_devices[index].gpios[i].name, 16, "gpio%d", i);
        }
        dev->remote_devices[index].gpios[i].mode = IOT_GPIO_MODE_OUTPUT;
    }

    IOT_Assign_Garden_GPIO_Names(&dev->remote_devices[index]);

    dev->remote_device_count++;

    if (dev->Log_Info) {
        dev->Log_Info(TAG, "Added remote device: %s (id=%d, gpios=%d)",
                      dev->remote_devices[index].device_name,
                      dev->remote_devices[index].device_id,
                      dev->remote_devices[index].gpio_count);
    }

    return index;
}

void IOT_Set_Local_GPIO(iot_controller_t* dev, int index, uint8_t level) {
    if (dev == NULL || index < 0 || index >= dev->local_gpio_count) {
        return;
    }

    if (dev->local_gpios[index].mode != IOT_GPIO_MODE_OUTPUT) {
        if (dev->Log_Error) {
            dev->Log_Error(TAG, "GPIO %d is not configured as output", index);
        }
        return;
    }

    if (dev->Gpio_Write) {
        dev->Gpio_Write(dev->local_gpios[index].port, dev->local_gpios[index].pin, level);
        dev->local_gpio_states[index] = level;
    }
}

int IOT_Get_Local_GPIO(iot_controller_t* dev, int index) {
    if (dev == NULL || index < 0 || index >= dev->local_gpio_count) {
        return -1;
    }

    if (dev->Gpio_Read) {
        return dev->Gpio_Read(dev->local_gpios[index].port, dev->local_gpios[index].pin);
    }

    return dev->local_gpio_states[index];
}

int IOT_Find_Device_By_Name(iot_controller_t* dev, const char* name) {
    if (dev == NULL || name == NULL) {
        return -1;
    }

    const char* resolved_name = IOT_Resolve_Alias(name);

    for (int i = 0; i < dev->remote_device_count; i++) {
        if (strcmp(dev->remote_devices[i].device_name, resolved_name) == 0) {
            return i;
        }
    }

    return -1;
}

int IOT_Find_Device_By_Mac(iot_controller_t* dev, const uint8_t* mac) {
    if (dev == NULL || mac == NULL) {
        return -1;
    }

    for (int i = 0; i < dev->remote_device_count; i++) {
        if (memcmp(dev->remote_devices[i].mac_addr, mac, 6) == 0) {
            return i;
        }
    }

    return -1;
}

void IOT_Update_Heartbeat(iot_controller_t* dev, const uint8_t* mac) {
    if (dev == NULL || mac == NULL) {
        return;
    }

    int index = IOT_Find_Device_By_Mac(dev, mac);
    if (index >= 0) {
        dev->remote_devices[index].online = true;
        dev->remote_devices[index].last_heartbeat_ms = (uint32_t)(esp_timer_get_time() / 1000);
    }
}

bool IOT_Set_Remote_GPIO(iot_controller_t* dev, int device_index, int gpio_index, uint8_t level) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return false;
    }

    if (gpio_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return false;
    }

    if (!dev->remote_devices[device_index].online) {
        if (dev->Log_Info) {
            dev->Log_Info(TAG, "Remote device %s is offline, trying anyway...",
                          dev->remote_devices[device_index].device_name);
        }
    }

    iot_command_packet_t cmd = {
        .command = IOT_CMD_SET_GPIO,
        .device_id = dev->remote_devices[device_index].device_id,
        .gpio_index = gpio_index,
        .value = level,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        char topic[64];
        snprintf(topic, sizeof(topic), MQTT_TOPIC_CMD_PREFIX "%d", dev->remote_devices[device_index].device_id);
        bool result = dev->Mqtt_Publish(topic, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            dev->remote_devices[device_index].gpio_states[gpio_index] = level;
        }
        if (dev->Log_Info) {
            dev->Log_Info(TAG, "Send to remote %s GPIO%d = %d, result=%d",
                          dev->remote_devices[device_index].device_name, gpio_index, level, result);
        }
        return result;
    }

    return false;
}

bool IOT_Set_Remote_Servo(iot_controller_t* dev, int device_index, int servo_index, uint8_t angle) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return false;
    }

    int gpio_index = IOT_Get_Servo_GPIO_Base(&dev->remote_devices[device_index]) + servo_index;
    if (servo_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return false;
    }

    iot_command_packet_t cmd = {
        .command = IOT_CMD_SET_SERVO,
        .device_id = dev->remote_devices[device_index].device_id,
        .gpio_index = gpio_index,
        .value = angle,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        char topic[64];
        snprintf(topic, sizeof(topic), MQTT_TOPIC_CMD_PREFIX "%d", dev->remote_devices[device_index].device_id);
        bool result = dev->Mqtt_Publish(topic, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            dev->remote_devices[device_index].gpio_states[gpio_index] = angle;
        }
        return result;
    }

    return false;
}

int IOT_Get_Remote_GPIO(iot_controller_t* dev, int device_index, int gpio_index) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return -1;
    }

    if (gpio_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return -1;
    }

    return dev->remote_devices[device_index].gpio_states[gpio_index];
}

void IOT_Process_Remote_Response(iot_controller_t* dev, const uint8_t* mac, const uint8_t* data, int len) {
    if (dev == NULL || data == NULL || len < (int)sizeof(iot_command_packet_t)) {
        ESP_LOGW(TAG, "Process_Remote_Response: invalid params, len=%d, sizeof=%d", len, (int)sizeof(iot_command_packet_t));
        return;
    }

    iot_command_packet_t* resp = (iot_command_packet_t*)data;

    int found_index = -1;
    for (int i = 0; i < dev->remote_device_count; i++) {
        if (memcmp(dev->remote_devices[i].mac_addr, mac, 6) == 0) {
            found_index = i;
            break;
        }
    }

    ESP_LOGI(TAG, "Process_Remote_Response: mac=%02X:%02X:%02X:%02X:%02X:%02X, cmd=%d, found=%d, len=%d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], resp->command, found_index, len);

    if (found_index >= 0) {
        dev->remote_devices[found_index].online = true;

        ESP_LOGI(TAG, "Device already exists [%s], checking for ANNOUNCE_V2...",
                 dev->remote_devices[found_index].device_name);

        if (resp->command == IOT_CMD_ANNOUNCE_V2 && len >= (int)sizeof(iot_announce_v2_packet_t)) {
            iot_announce_v2_packet_t* announce = (iot_announce_v2_packet_t*)data;
            ESP_LOGI(TAG, ">>> ANNOUNCE_V2 received: name='%s', gpio_index=%d, len=%d",
                     announce->device_name, announce->gpio_index, len);
            strncpy(dev->remote_devices[found_index].device_name, announce->device_name, IOT_DEVICE_NAME_LEN - 1);
            dev->remote_devices[found_index].device_name[IOT_DEVICE_NAME_LEN - 1] = '\0';
            uint8_t new_gpio_count = announce->gpio_index > 0 ?
                (announce->gpio_index > IOT_MAX_REMOTE_GPIOS ? IOT_MAX_REMOTE_GPIOS : announce->gpio_index) : 4;
            if (new_gpio_count > dev->remote_devices[found_index].gpio_count) {
                dev->remote_devices[found_index].gpio_count = new_gpio_count;
            }
            dev->remote_devices[found_index].device_id =
                IOT_Get_Device_Id_By_Name(dev->remote_devices[found_index].device_name);
            IOT_Assign_Garden_GPIO_Names(&dev->remote_devices[found_index]);
            if (dev->Log_Info) {
                dev->Log_Info(TAG, "Updated device info: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X, GPIOs: %d)",
                              announce->device_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                              dev->remote_devices[found_index].gpio_count);
            }
        } else if (resp->command == IOT_CMD_RESPONSE) {
            if (resp->gpio_index < dev->remote_devices[found_index].gpio_count) {
                dev->remote_devices[found_index].gpio_states[resp->gpio_index] = resp->value;
                ESP_LOGI(TAG, "GPIO state updated: %s[%d]=%d (origin=%d)",
                         dev->remote_devices[found_index].device_name,
                         resp->gpio_index, resp->value, resp->reserved[0]);
            }
        } else {
            ESP_LOGW(TAG, ">>> Not ANNOUNCE_V2: cmd=%d, len=%d < sizeof(iot_announce_v2_packet_t)=%d",
                     resp->command, len, (int)sizeof(iot_announce_v2_packet_t));
        }
    } else if (resp->command == IOT_CMD_ANNOUNCE) {
        int gpio_count = resp->gpio_index > 0 ? resp->gpio_index : 4;
        uint8_t has_servo = resp->value & 0x01;

        char device_name[IOT_DEVICE_NAME_LEN];
        snprintf(device_name, sizeof(device_name), "未知设备");

        int new_index = IOT_Add_Remote_Device(dev, device_name, mac, gpio_count);
        if (new_index >= 0) {
            dev->remote_devices[new_index].online = true;
            if (dev->Log_Info) {
                dev->Log_Info(TAG, "New device discovered: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X, GPIOs: %d, Servo: %s)",
                              device_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                              gpio_count, has_servo ? "Yes" : "No");
            }
        }
    } else if (resp->command == IOT_CMD_ANNOUNCE_V2 && len >= (int)sizeof(iot_announce_v2_packet_t)) {
        iot_announce_v2_packet_t* announce = (iot_announce_v2_packet_t*)data;
        int gpio_count = announce->gpio_index > 0 ? announce->gpio_index : 4;

        int new_index = IOT_Add_Remote_Device(dev, announce->device_name, mac, gpio_count);
        if (new_index >= 0) {
            dev->remote_devices[new_index].online = true;
            if (dev->Log_Info) {
                dev->Log_Info(TAG, "New device discovered: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X, GPIOs: %d)",
                              announce->device_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                              gpio_count);
            }
        }
    }
}

void IOT_Send_Heartbeat(iot_controller_t* dev) {
    if (dev == NULL) return;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_HEARTBEAT,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
    }
}

void IOT_Discover_Devices(iot_controller_t* dev) {
    if (dev == NULL) return;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_DISCOVER,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        bool result = dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));

        if (dev->Log_Info) {
            dev->Log_Info(TAG, "Sent discover command (result=%d, size=%d)", result, (int)sizeof(cmd));
        }
    } else {
        if (dev->Log_Error) {
            dev->Log_Error(TAG, "Mqtt_Publish not initialized!");
        }
    }
}

void IOT_Controller_Init(iot_controller_t* dev) {
    if (dev == NULL) return;
    dev->remote_device_count = 0;
    dev->local_gpio_count = 0;
}

bool IOT_Set_Light(iot_controller_t* dev, int device_index, uint8_t light_index, uint8_t on) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return false;
    }
    int gpio_index = light_index;
    if (gpio_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return false;
    }
    return IOT_Set_Remote_GPIO(dev, device_index, gpio_index, on);
}

bool IOT_Set_Relay(iot_controller_t* dev, int device_index, uint8_t relay_index, uint8_t on) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return false;
    }
    int gpio_index = GARDEN_LIGHT_COUNT + relay_index;
    if (gpio_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return false;
    }
    return IOT_Set_Remote_GPIO(dev, device_index, gpio_index, on);
}

bool IOT_Broadcast_All_Off(iot_controller_t* dev) {
    if (dev == NULL) return false;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_BROADCAST_ALL_OFF,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        bool result = dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            for (int d = 0; d < dev->remote_device_count; d++) {
                for (int g = 0; g < dev->remote_devices[d].gpio_count; g++) {
                    dev->remote_devices[d].gpio_states[g] = 0;
                }
            }
        }
        return result;
    }
    return false;
}

bool IOT_Broadcast_All_On(iot_controller_t* dev) {
    if (dev == NULL) return false;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_BROADCAST_ALL_ON,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        bool result = dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            for (int d = 0; d < dev->remote_device_count; d++) {
                for (int g = 0; g < dev->remote_devices[d].gpio_count; g++) {
                    dev->remote_devices[d].gpio_states[g] = 1;
                }
            }
        }
        return result;
    }
    return false;
}

bool IOT_Broadcast_Lights_Off(iot_controller_t* dev) {
    if (dev == NULL) return false;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_BROADCAST_LIGHTS_OFF,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        bool result = dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            for (int d = 0; d < dev->remote_device_count; d++) {
                for (int g = 0; g < 3 && g < dev->remote_devices[d].gpio_count; g++) {
                    dev->remote_devices[d].gpio_states[g] = 0;
                }
            }
        }
        return result;
    }
    return false;
}

bool IOT_Broadcast_Lights_On(iot_controller_t* dev) {
    if (dev == NULL) return false;

    iot_command_packet_t cmd = {
        .command = IOT_CMD_BROADCAST_LIGHTS_ON,
        .device_id = 0,
        .gpio_index = 0,
        .value = 0,
        .reserved = {0}
    };

    if (dev->Mqtt_Publish) {
        bool result = dev->Mqtt_Publish(MQTT_TOPIC_CMD_BROADCAST, (const uint8_t*)&cmd, sizeof(cmd));
        if (result) {
            for (int d = 0; d < dev->remote_device_count; d++) {
                for (int g = 0; g < 3 && g < dev->remote_devices[d].gpio_count; g++) {
                    dev->remote_devices[d].gpio_states[g] = 1;
                }
            }
        }
        return result;
    }
    return false;
}

void IOT_Check_Device_Timeout(iot_controller_t* dev, uint32_t timeout_ms) {
    if (dev == NULL) return;

    uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);

    for (int i = 0; i < dev->remote_device_count; i++) {
        if (!dev->remote_devices[i].online) continue;

        uint32_t elapsed = (uint32_t)(current_time_ms - dev->remote_devices[i].last_heartbeat_ms);
        if (elapsed > timeout_ms) {
            dev->remote_devices[i].online = false;
            IOT_Log_Event(dev, "%s 离线", dev->remote_devices[i].device_name);
            if (dev->Log_Info) {
                dev->Log_Info(TAG, "Device %s timed out", dev->remote_devices[i].device_name);
            }
        }
    }
}

bool IOT_Set_Servo(iot_controller_t* dev, int device_index, uint8_t servo_index, uint8_t angle) {
    if (dev == NULL || device_index < 0 || device_index >= dev->remote_device_count) {
        return false;
    }
    int gpio_index = IOT_Get_Servo_GPIO_Base(&dev->remote_devices[device_index]) + servo_index;
    if (gpio_index < 0 || gpio_index >= dev->remote_devices[device_index].gpio_count) {
        return false;
    }
    return IOT_Set_Remote_Servo(dev, device_index, servo_index, angle);
}

void IOT_Process_Sensor_Data(iot_controller_t* dev, const uint8_t* mac, const uint8_t* data, int len) {
    if (dev == NULL || mac == NULL || data == NULL) return;

    int idx = IOT_Find_Device_By_Mac(dev, mac);
    if (idx < 0) return;

    iot_sensor_state_t* s = &dev->remote_devices[idx].sensors;
    s->last_update_ms = (uint32_t)(esp_timer_get_time() / 1000);

    if (data[0] == '{') {
        cJSON* root = cJSON_ParseWithLength((const char*)data, len);
        if (root) {
            cJSON* item;
            item = cJSON_GetObjectItem(root, "soil_mv");
            if (item) s->soil_mv = (uint16_t)item->valueint;
            item = cJSON_GetObjectItem(root, "rain_mv");
            if (item) s->rain_adc_mv = (uint16_t)item->valueint;
            item = cJSON_GetObjectItem(root, "fire_status");
            if (item) s->fire_status = (uint8_t)item->valueint;
            item = cJSON_GetObjectItem(root, "rain_status");
            if (item) s->rain_status = (uint8_t)item->valueint;
            item = cJSON_GetObjectItem(root, "help_status");
            if (item) s->help_status = (uint8_t)item->valueint;
            item = cJSON_GetObjectItem(root, "temperature");
            if (item) s->temperature = (float)item->valuedouble;
            item = cJSON_GetObjectItem(root, "humidity");
            if (item) s->humidity = (float)item->valuedouble;
            cJSON_Delete(root);
        }
        return;
    }

    if (len < (int)sizeof(iot_command_packet_t)) return;

    iot_command_packet_t* pkt = (iot_command_packet_t*)data;
    if (pkt->command != IOT_CMD_SENSOR_REPORT) return;

    uint16_t value_16 = (uint16_t)pkt->value | ((uint16_t)pkt->reserved[0] << 8);

    switch (pkt->gpio_index) {
        case IOT_SENSOR_SOIL_MV:     s->soil_mv = value_16; break;
        case IOT_SENSOR_FIRE_STATUS: s->fire_status = pkt->value; break;
        case IOT_SENSOR_RAIN_STATUS: s->rain_status = pkt->value; break;
        case IOT_SENSOR_HELP_STATUS: s->help_status = pkt->value; break;
        case IOT_SENSOR_RAIN_ADC_MV: s->rain_adc_mv = value_16; break;
    }
}

void IOT_Get_Sensors_Json(iot_controller_t* dev, char* buffer, int buffer_size) {
    if (dev == NULL || buffer == NULL || buffer_size <= 0) return;

    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\"devices\":[");

    bool first = true;
    for (int i = 0; i < dev->remote_device_count && offset < buffer_size - 200; i++) {
        iot_remote_device_t* d = &dev->remote_devices[i];
        iot_sensor_state_t* s = &d->sensors;

        if (!first) offset += snprintf(buffer + offset, buffer_size - offset, ",");
        first = false;

        offset += snprintf(buffer + offset, buffer_size - offset,
            "{\"name\":\"%s\",\"online\":%s,\"soil_mv\":%d,\"rain_adc_mv\":%d,"
            "\"fire_status\":%d,\"rain_status\":%d,\"help_status\":%d,"
            "\"temperature\":%.1f,\"humidity\":%.1f,\"updated_ms\":%lu}",
            d->device_name,
            d->online ? "true" : "false",
            s->soil_mv, s->rain_adc_mv,
            s->fire_status, s->rain_status, s->help_status,
            s->temperature, s->humidity,
            (unsigned long)s->last_update_ms);
    }

    offset += snprintf(buffer + offset, buffer_size - offset, "]}");
}

void IOT_Log_Event(iot_controller_t* dev, const char* fmt, ...) {
    if (dev == NULL || fmt == NULL) return;

    char* entry = dev->event_log[dev->event_log_head];
    va_list args;
    va_start(args, fmt);
    vsnprintf(entry, IOT_EVENT_STR_LEN, fmt, args);
    va_end(args);
    entry[IOT_EVENT_STR_LEN - 1] = '\0';

    dev->event_log_head = (dev->event_log_head + 1) % IOT_EVENT_LOG_MAX;
    if (dev->event_log_count < IOT_EVENT_LOG_MAX) {
        dev->event_log_count++;
    }
}

void IOT_Get_Event_Log_Json(iot_controller_t* dev, char* buffer, int buffer_size) {
    if (dev == NULL || buffer == NULL || buffer_size <= 0) return;

    int offset = 0;
    offset += snprintf(buffer + offset, buffer_size - offset, "{\"events\":[");

    int start = dev->event_log_count < IOT_EVENT_LOG_MAX ? 0 :
                dev->event_log_head;
    for (int i = 0; i < dev->event_log_count && offset < buffer_size - 100; i++) {
        int idx = (start + i) % IOT_EVENT_LOG_MAX;
        if (i > 0) offset += snprintf(buffer + offset, buffer_size - offset, ",");
        offset += snprintf(buffer + offset, buffer_size - offset,
                          "{\"idx\":%d,\"msg\":\"%s\"}", i, dev->event_log[idx]);
    }

    offset += snprintf(buffer + offset, buffer_size - offset, "]}");
}
