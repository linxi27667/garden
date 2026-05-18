#include "alarm_mcp_tool.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <cJSON.h>

static const char* TAG = "ALARM_MCP";

static alarm_manager_t g_alarm_manager = {0};
static bool g_initialized = false;
static alarm_callback_t g_alarm_callback = NULL;

static const char* NVS_NAMESPACE = "alarm";

static void Save_Alarms_To_NVS(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", NVS_NAMESPACE);
        return;
    }
    
    size_t required_size = sizeof(g_alarm_manager.alarms);
    nvs_set_blob(nvs_handle, "alarms", g_alarm_manager.alarms, required_size);
    nvs_set_i32(nvs_handle, "alarm_count", g_alarm_manager.alarm_count);
    nvs_set_i32(nvs_handle, "next_id", g_alarm_manager.next_id);
    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Alarms saved to NVS");
}

static void Load_Alarms_From_NVS(void) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS namespace not found, using defaults");
        return;
    }
    
    size_t required_size = sizeof(g_alarm_manager.alarms);
    size_t actual_size = 0;
    err = nvs_get_blob(nvs_handle, "alarms", NULL, &actual_size);
    if (err == ESP_OK && actual_size != required_size) {
        ESP_LOGW(TAG, "NVS blob size mismatch (expected=%u, actual=%u), resetting to defaults",
                 (unsigned)required_size, (unsigned)actual_size);
        nvs_close(nvs_handle);
        return;
    }
    err = nvs_get_blob(nvs_handle, "alarms", g_alarm_manager.alarms, &required_size);
    if (err == ESP_OK) {
        int32_t count = 0;
        int32_t next_id = 1;
        nvs_get_i32(nvs_handle, "alarm_count", &count);
        nvs_get_i32(nvs_handle, "next_id", &next_id);
        g_alarm_manager.alarm_count = count;
        g_alarm_manager.next_id = next_id;
        
        for (int i = 0; i < g_alarm_manager.alarm_count && i < MAX_ALARM_COUNT; i++) {
            g_alarm_manager.alarms[i].message[ALARM_MESSAGE_MAX_LEN - 1] = '\0';
            g_alarm_manager.alarms[i].medicine_name[31] = '\0';
        }
        
        ESP_LOGI(TAG, "Loaded %d alarms from NVS", g_alarm_manager.alarm_count);
    }
    
    nvs_close(nvs_handle);
}

bool Alarm_Mcp_Init(void) {
    if (g_initialized) {
        ESP_LOGW(TAG, "Alarm manager already initialized");
        return true;
    }
    
    ESP_LOGI(TAG, "Initializing Alarm MCP tool...");
    
    memset(&g_alarm_manager, 0, sizeof(g_alarm_manager));
    g_alarm_manager.next_id = 1;
    
    Load_Alarms_From_NVS();
    
    g_initialized = true;
    ESP_LOGI(TAG, "Alarm MCP tool initialized with %d alarms", g_alarm_manager.alarm_count);
    
    return true;
}

static int Add_Alarm_Internal(alarm_type_t type, int64_t trigger_time, 
                               const char* message, const char* medicine_name,
                               bool repeat_daily) {
    if (g_alarm_manager.alarm_count >= MAX_ALARM_COUNT) {
        ESP_LOGE(TAG, "Alarm list is full");
        return -1;
    }
    
    if (message == NULL || strlen(message) == 0) {
        ESP_LOGE(TAG, "Invalid alarm message");
        return -2;
    }
    
    time_t now;
    time(&now);
    
    int index = g_alarm_manager.alarm_count;
    alarm_info_t* alarm = &g_alarm_manager.alarms[index];
    
    alarm->id = g_alarm_manager.next_id++;
    alarm->type = type;
    alarm->state = ALARM_STATE_PENDING;
    alarm->trigger_time = trigger_time;
    alarm->created_time = (int64_t)now;
    strncpy(alarm->message, message, ALARM_MESSAGE_MAX_LEN - 1);
    alarm->message[ALARM_MESSAGE_MAX_LEN - 1] = '\0';
    
    if (medicine_name) {
        strncpy(alarm->medicine_name, medicine_name, 31);
        alarm->medicine_name[31] = '\0';
    } else {
        alarm->medicine_name[0] = '\0';
    }
    
    alarm->repeat_daily = repeat_daily;
    alarm->enabled = true;

    g_alarm_manager.alarm_count++;
    
    Save_Alarms_To_NVS();

    struct tm tm_buf;
    struct tm* tm_info = localtime_r((time_t*)&trigger_time, &tm_buf);
    if (tm_info) {
        ESP_LOGI(TAG, "Added alarm: id=%d, type=%d, trigger=%04d-%02d-%02d %02d:%02d, message=%s",
                 alarm->id, type, tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                 tm_info->tm_hour, tm_info->tm_min, message);
    } else {
        ESP_LOGI(TAG, "Added alarm: id=%d, type=%d, message=%s",
                 alarm->id, type, message);
    }

    return alarm->id;
}

int Alarm_Mcp_Add_Timer(int minutes, const char* message) {
    if (minutes <= 0) {
        ESP_LOGE(TAG, "Invalid timer minutes: %d", minutes);
        return -1;
    }

    time_t now;
    time(&now);
    int64_t trigger_time = (int64_t)now + (minutes * 60);

    return Add_Alarm_Internal(ALARM_TYPE_TIMER, trigger_time, message, NULL, false);
}

int Alarm_Mcp_Add_Timer_Seconds(int seconds, const char* message) {
    if (seconds <= 0) {
        ESP_LOGE(TAG, "Invalid timer seconds: %d", seconds);
        return -1;
    }

    time_t now;
    time(&now);
    int64_t trigger_time = (int64_t)now + seconds;

    return Add_Alarm_Internal(ALARM_TYPE_TIMER, trigger_time, message, NULL, false);
}

int Alarm_Mcp_Add_Alarm(int hour, int minute, const char* message, bool repeat_daily) {
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        ESP_LOGE(TAG, "Invalid alarm time: %02d:%02d", hour, minute);
        return -1;
    }

    time_t now;
    time(&now);
    struct tm tm_now_buf;
    struct tm* tm_now = localtime_r(&now, &tm_now_buf);

    struct tm tm_trigger = *tm_now;
    tm_trigger.tm_hour = hour;
    tm_trigger.tm_min = minute;
    tm_trigger.tm_sec = 0;

    time_t trigger_time = mktime(&tm_trigger);

    if (trigger_time <= now) {
        trigger_time += 24 * 60 * 60;
    }

    return Add_Alarm_Internal(ALARM_TYPE_ALARM, (int64_t)trigger_time, message, NULL, repeat_daily);
}

bool Alarm_Mcp_Remove(int alarm_id) {
    for (int i = 0; i < g_alarm_manager.alarm_count; i++) {
        if (g_alarm_manager.alarms[i].id == alarm_id) {
            for (int j = i; j < g_alarm_manager.alarm_count - 1; j++) {
                g_alarm_manager.alarms[j] = g_alarm_manager.alarms[j + 1];
            }
            g_alarm_manager.alarm_count--;
            Save_Alarms_To_NVS();
            ESP_LOGI(TAG, "Removed alarm: id=%d", alarm_id);
            return true;
        }
    }
    
    ESP_LOGW(TAG, "Alarm not found: id=%d", alarm_id);
    return false;
}

bool Alarm_Mcp_Dismiss(int alarm_id) {
    alarm_info_t* alarm = Alarm_Mcp_Get_By_Id(alarm_id);
    if (alarm == NULL) {
        return false;
    }
    
    alarm->state = ALARM_STATE_DISMISSED;
    alarm->enabled = false;
    
    if (alarm->repeat_daily) {
        alarm->trigger_time += 24 * 60 * 60;
        alarm->state = ALARM_STATE_PENDING;
        alarm->enabled = true;
        ESP_LOGI(TAG, "Alarm %d dismissed, rescheduled for tomorrow", alarm_id);
    } else {
        ESP_LOGI(TAG, "Alarm %d dismissed", alarm_id);
    }
    
    Save_Alarms_To_NVS();
    return true;
}

alarm_info_t* Alarm_Mcp_Get_By_Id(int alarm_id) {
    for (int i = 0; i < g_alarm_manager.alarm_count; i++) {
        if (g_alarm_manager.alarms[i].id == alarm_id) {
            return &g_alarm_manager.alarms[i];
        }
    }
    return NULL;
}

int Alarm_Mcp_Get_All(alarm_info_t* alarms, int max_count) {
    if (alarms == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = (g_alarm_manager.alarm_count < max_count) ? 
                g_alarm_manager.alarm_count : max_count;
    
    memcpy(alarms, g_alarm_manager.alarms, count * sizeof(alarm_info_t));
    return count;
}

int Alarm_Mcp_Get_Pending(alarm_info_t* alarms, int max_count) {
    if (alarms == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < g_alarm_manager.alarm_count && count < max_count; i++) {
        if (g_alarm_manager.alarms[i].enabled && 
            g_alarm_manager.alarms[i].state == ALARM_STATE_PENDING) {
            alarms[count] = g_alarm_manager.alarms[i];
            count++;
        }
    }
    
    return count;
}

int Alarm_Mcp_Get_Triggered(alarm_info_t* alarms, int max_count) {
    if (alarms == NULL || max_count <= 0) {
        return 0;
    }
    
    int count = 0;
    for (int i = 0; i < g_alarm_manager.alarm_count && count < max_count; i++) {
        if (g_alarm_manager.alarms[i].state == ALARM_STATE_TRIGGERED) {
            alarms[count] = g_alarm_manager.alarms[i];
            count++;
        }
    }
    
    return count;
}

void Alarm_Mcp_Set_Callback(alarm_callback_t callback) {
    g_alarm_callback = callback;
}

void Alarm_Mcp_Process(void) {
    time_t now;
    time(&now);
    int64_t current_time = (int64_t)now;
    
    for (int i = 0; i < g_alarm_manager.alarm_count; i++) {
        alarm_info_t* alarm = &g_alarm_manager.alarms[i];
        
        if (alarm->enabled && alarm->state == ALARM_STATE_PENDING) {
            if (current_time >= alarm->trigger_time) {
                ESP_LOGI(TAG, "Alarm triggered: id=%d, message=%s", alarm->id, alarm->message);
                alarm->state = ALARM_STATE_TRIGGERED;
                
                if (g_alarm_callback) {
                    g_alarm_callback(alarm);
                }
            }
        }
    }
}

void Alarm_Mcp_Get_Status_Json(char* buffer, int buffer_size) {
    if (buffer == NULL || buffer_size <= 0) return;

    time_t now;
    time(&now);

    cJSON* root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddNumberToObject(root, "current_time", (double)now);

    cJSON* alarms_array = cJSON_CreateArray();
    if (!alarms_array) {
        cJSON_Delete(root);
        return;
    }
    cJSON_AddItemToObject(root, "alarms", alarms_array);

    for (int i = 0; i < g_alarm_manager.alarm_count && i < MAX_ALARM_COUNT; i++) {
        alarm_info_t* alarm = &g_alarm_manager.alarms[i];

        alarm->message[ALARM_MESSAGE_MAX_LEN - 1] = '\0';
        alarm->medicine_name[31] = '\0';

        cJSON* alarm_json = cJSON_CreateObject();
        if (!alarm_json) continue;

        cJSON_AddNumberToObject(alarm_json, "id", alarm->id);
        cJSON_AddNumberToObject(alarm_json, "type", alarm->type);
        cJSON_AddNumberToObject(alarm_json, "state", alarm->state);
        cJSON_AddNumberToObject(alarm_json, "trigger_time", (double)alarm->trigger_time);

        struct tm tm_buf = {0};
        struct tm* tm_info = localtime_r((time_t*)&alarm->trigger_time, &tm_buf);

        char time_str[64] = {0};
        if (tm_info) {
            snprintf(time_str, sizeof(time_str), "%04d-%02d-%02d %02d:%02d",
                     tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                     tm_info->tm_hour, tm_info->tm_min);
        }
        cJSON_AddStringToObject(alarm_json, "trigger_str", time_str);

        cJSON_AddStringToObject(alarm_json, "message", alarm->message);
        cJSON_AddStringToObject(alarm_json, "medicine_name", alarm->medicine_name);
        cJSON_AddBoolToObject(alarm_json, "repeat_daily", alarm->repeat_daily);
        cJSON_AddBoolToObject(alarm_json, "enabled", alarm->enabled);

        cJSON_AddItemToArray(alarms_array, alarm_json);
    }

    char* json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        strncpy(buffer, json_str, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        free(json_str);
    } else {
        buffer[0] = '\0';
    }

    cJSON_Delete(root);
}

alarm_manager_t* Alarm_Mcp_Get_Manager(void) {
    return &g_alarm_manager;
}
