#ifndef __MQTT_RECEIVE_H__
#define __MQTT_RECEIVE_H__

#include "mqtt_client.h"
#include "esp_event.h"

/**
 * 紧急事件类型
 */
typedef enum {
    EMERGENCY_FIRE = 0,
    EMERGENCY_HELP,
    EMERGENCY_RAIN,
    EMERGENCY_COUNT
} emergency_type_t;

/**
 * 注册紧急事件回调 - 用于从机向主控上报火灾/求助等紧急事件
 * 回调在MQTT任务上下文中执行，不应执行耗时操作
 */
typedef void (*emergency_event_cb_t)(emergency_type_t type, const char* detail);

void MQTT_Receive_Init(void);
esp_mqtt_client_handle_t MQTT_Get_Client(void);
const char* MQTT_Get_MAC_Str(void);
bool MQTT_Is_Connected(void);

/**
 * 注册紧急事件回调
 */
void MQTT_Register_Emergency_Callback(emergency_event_cb_t cb);

/**
 * 触发紧急事件上报 - 线程安全，可从ISR或任何任务调用
 * 将紧急事件发布到MQTT主题 xiaozhi/iot/emergency/<mac>
 */
void MQTT_Publish_Emergency(emergency_type_t type, const char* detail);

#endif
