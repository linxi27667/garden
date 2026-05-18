#ifndef ALARM_MCP_TOOL_H
#define ALARM_MCP_TOOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ALARM_MESSAGE_MAX_LEN   128
#define MAX_ALARM_COUNT         20

typedef enum {
    ALARM_TYPE_TIMER = 0,       // 定时器（X分钟后触发）
    ALARM_TYPE_ALARM = 1,       // 闹钟（指定时间触发）
} alarm_type_t;

typedef enum {
    ALARM_STATE_PENDING = 0,    // 等待触发
    ALARM_STATE_TRIGGERED = 1,  // 已触发
    ALARM_STATE_DISMISSED = 2,  // 已关闭
} alarm_state_t;

typedef struct {
    int id;
    alarm_type_t type;
    alarm_state_t state;
    int64_t trigger_time;       // 触发时间戳（秒）
    int64_t created_time;       // 创建时间戳
    char message[ALARM_MESSAGE_MAX_LEN];
    char medicine_name[32];     // 保留字段（NVS兼容，不再使用）
    bool repeat_daily;          // 是否每天重复
    bool enabled;
} alarm_info_t;

typedef struct {
    alarm_info_t alarms[MAX_ALARM_COUNT];
    int alarm_count;
    int next_id;
} alarm_manager_t;

typedef void (*alarm_callback_t)(alarm_info_t* alarm);

bool Alarm_Mcp_Init(void);

int Alarm_Mcp_Add_Timer(int minutes, const char* message);

int Alarm_Mcp_Add_Timer_Seconds(int seconds, const char* message);

int Alarm_Mcp_Add_Alarm(int hour, int minute, const char* message, bool repeat_daily);

bool Alarm_Mcp_Remove(int alarm_id);

bool Alarm_Mcp_Dismiss(int alarm_id);

alarm_info_t* Alarm_Mcp_Get_By_Id(int alarm_id);

int Alarm_Mcp_Get_All(alarm_info_t* alarms, int max_count);

int Alarm_Mcp_Get_Pending(alarm_info_t* alarms, int max_count);

int Alarm_Mcp_Get_Triggered(alarm_info_t* alarms, int max_count);

void Alarm_Mcp_Set_Callback(alarm_callback_t callback);

void Alarm_Mcp_Process(void);

void Alarm_Mcp_Get_Status_Json(char* buffer, int buffer_size);

alarm_manager_t* Alarm_Mcp_Get_Manager(void);

#ifdef __cplusplus
}
#endif

#endif
