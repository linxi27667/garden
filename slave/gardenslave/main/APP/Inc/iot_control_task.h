#ifndef IOT_CONTROL_TASK_H
#define IOT_CONTROL_TASK_H

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define LIGHT_COUNT    3
#define RELAY_COUNT    4
#define SERVO_COUNT    1

typedef enum {
    OFF = 0,
    ON  = 1
} device_onoff_enum_t;

typedef enum {
     SERVO_MIN_ANGLE = 0,
     SERVO_MAX_ANGLE = 180
} servo_angle_limits_t;

typedef int16_t servo_angle_t;

typedef enum {
    FIRE_NORMAL   = 0,
    FIRE_WARNING  = 1,
    FIRE_CONFIRMED = 2
} fire_state_enum_t;

#define LIGHT_1_CHINESE   "西庭灯"
#define LIGHT_2_CHINESE   "庭院灯带"
#define LIGHT_3_CHINESE   "长廊灯"
#define RELAY_1_CHINESE   "风扇"
#define RELAY_2_CHINESE   "水泵"
#define RELAY_3_CHINESE   "雾化器"
#define RELAY_4_CHINESE   "安防总闸"
#define SERVO_1_CHINESE   "晾衣架"

typedef struct {
     device_onoff_enum_t light[LIGHT_COUNT];
     device_onoff_enum_t relay[RELAY_COUNT];
     servo_angle_t       servo[SERVO_COUNT];
     fire_state_enum_t   fire_status;
     bool                auto_mode;
} device_flags_enum_t;

extern volatile device_flags_enum_t g_device_flags;
extern SemaphoreHandle_t g_device_flags_mutex;

#define DEVICE_FLAGS_LOCK()   do { xSemaphoreTake(g_device_flags_mutex, portMAX_DELAY); } while(0)
#define DEVICE_FLAGS_UNLOCK() do { xSemaphoreGive(g_device_flags_mutex); } while(0)

void Iot_Control_Task_Init(void);

void Iot_Fire_Emergency_Response(void);
void Iot_Help_Emergency_Response(void);
void Iot_Rain_Alert_Response(void);

extern volatile bool g_rain_alert_triggered;
extern volatile bool g_help_detected;

#endif
