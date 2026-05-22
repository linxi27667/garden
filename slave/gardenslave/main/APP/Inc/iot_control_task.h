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
    SWITCH_OFF = 0,
    SWITCH_ON  = 1
} device_switch_state_t;

typedef enum {
     SERVO_MIN_ANGLE = 0,
     SERVO_MAX_ANGLE = 180
} servo_angle_limit_t;

typedef int16_t servo_angle_t;

typedef enum {
    FIRE_NORMAL   = 0,
    FIRE_WARNING  = 1,
    FIRE_CONFIRMED = 2
} fire_state_enum_t;

#define LIGHT_COURTYARD_WEST_NAME   "西庭灯"
#define LIGHT_STRIP_NAME            "庭院灯带"
#define LIGHT_CORRIDOR_LONG_NAME    "长廊灯"
#define RELAY_FAN_NAME              "风扇"
#define RELAY_WATER_PUMP_NAME       "水泵"
#define RELAY_ATOMIZER_NAME         "雾化器"
#define RELAY_SAFETY_SWITCH_NAME    "安防总闸"
#define SERVO_CLOTHES_NAME          "晾衣架"

typedef struct {
     device_switch_state_t light[LIGHT_COUNT];
     device_switch_state_t relay[RELAY_COUNT];
     servo_angle_t       servo[SERVO_COUNT];
     fire_state_enum_t   fire_status;
     bool                auto_mode;
     uint8_t             strip_red;
     uint8_t             strip_green;
     uint8_t             strip_blue;
     uint8_t             strip_brightness;
     uint8_t             strip_effect;
     uint8_t             ambience_scene;
} device_flags_enum_t;

extern volatile device_flags_enum_t g_device_flags;
extern SemaphoreHandle_t g_device_flags_mutex;

bool Iot_Device_Flags_Lock(const char* caller);
void Iot_Device_Flags_Unlock(const char* caller);

#define DEVICE_FLAGS_LOCK()   do { if (!Iot_Device_Flags_Lock(__func__)) { return; } } while(0)
#define DEVICE_FLAGS_UNLOCK() do { Iot_Device_Flags_Unlock(__func__); } while(0)

void Iot_Control_Task_Init(void);

void Iot_Fire_Emergency_Response(void);
void Iot_Help_Emergency_Response(void);
void Iot_Rain_Alert_Response(void);

#define NOTIFY_FIRE_BIT       (1UL << 0)
#define NOTIFY_HELP_BIT       (1UL << 1)
#define NOTIFY_RAIN_BIT       (1UL << 2)

extern TaskHandle_t s_iot_ctrl_task_handle;
extern volatile bool g_help_detected;

#endif
