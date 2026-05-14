#ifndef __IOT_CONTROLLER_H__
#define __IOT_CONTROLLER_H__

#include <stdint.h>
#include <stdbool.h>

/* ================= 中文设备名称宏（用于 MCP 意图识别） ================= */
#define CHINESE_LIGHT_NAMES(n) \
    "一楼设备:大厅灯/待定/待定; " \
    "二楼设备:主卧灯/客厅灯/厕所灯; " \
    "三楼设备:阳台灯/待定/待定"

#define CHINESE_RELAY_NAMES(n) \
    "一楼设备:待定/待定/待定; " \
    "二楼设备:风扇/待定/待定; " \
    "三楼设备:待定/待定/待定"

#define CHINESE_SERVO_NAMES(n) \
    "一楼设备:大门/待定/待定; " \
    "二楼设备:晾衣架/待定/待定; " \
    "三楼设备:左天窗/右天窗/晾衣架"

#ifdef __cplusplus
extern "C" {
#endif

void App_IOT_System_Init(void);
bool App_IOT_Set_GPIO(const char* device_name, const char* gpio_name, uint8_t level);
int App_IOT_Get_GPIO(const char* device_name, const char* gpio_name);
int App_IOT_Get_Device_Count(void);
int App_IOT_Get_Local_GPIO_Count(void);
const char* App_IOT_Get_Device_Name(int index);
const char* App_IOT_Get_Local_GPIO_Name(int index);
bool App_IOT_Add_Remote_Device(const char* name, const uint8_t* mac, int gpio_count);
void App_IOT_Discover_Devices(void);
bool App_IOT_Set_Servo(const char* device_name, const char* gpio_name, uint8_t angle);
bool App_IOT_Set_Light(const char* device_name, uint8_t light_index, bool on);
bool App_IOT_Set_Relay(const char* device_name, uint8_t relay_index, bool on);
bool App_IOT_Set_Servo_ByIndex(const char* device_name, uint8_t servo_index, uint8_t angle);
bool App_IOT_Broadcast_All_Off(void);
bool App_IOT_Broadcast_All_On(void);
bool App_IOT_Broadcast_Lights_Off(void);
bool App_IOT_Broadcast_Lights_On(void);
void App_IOT_Get_All_Status(char* buffer, int buffer_size);

bool App_IOT_Scenario_Homecoming(const char* first_floor_device, const char* all_floors_device);
bool App_IOT_Scenario_LeaveHome(const char* first_floor_device);
bool App_IOT_Scenario_GoodNight(const char* all_floors_device);
bool App_IOT_Scenario_Emergency(void);
bool App_IOT_Fire_Reset(void);
bool App_IOT_Control_Servo_Angle(const char* device_name, uint8_t servo_index, uint8_t angle);
bool App_IOT_Scenario_Rain_Collect_Clothes(const char* third_floor_device);
bool App_IOT_Scenario_Ventilate(const char* third_floor_device, uint8_t window_angle, uint8_t fan_on);
void App_IOT_Get_Sensors_Json(char* buffer, int buffer_size);
void App_IOT_Get_Event_Log_Json(char* buffer, int buffer_size);

bool App_IOT_MQTT_Publish(const char* topic, const char* data, int len);
bool App_IOT_MQTT_Subscribe(const char* topic);

typedef void (*iot_mqtt_cmd_callback_t)(const char* topic, const char* data, int data_len);
typedef void (*iot_emergency_alert_cb_t)(const char* type, const char* detail);

void App_IOT_Register_Mqtt_Cmd_Callback(const char* topic_prefix, iot_mqtt_cmd_callback_t callback);
void App_IOT_Register_Emergency_Alert_Callback(iot_emergency_alert_cb_t cb);

void App_IOT_Emergency_Alert(const char* type, const char* detail);

#ifdef __cplusplus
}
#endif

#endif
