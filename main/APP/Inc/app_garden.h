#ifndef __APP_GARDEN_H__
#define __APP_GARDEN_H__

#include <stdint.h>
#include <stdbool.h>
#include "bsp_garden.h"

#ifdef __cplusplus
extern "C" {
#endif

void App_Garden_Init(void);
void App_Garden_Start(void);

garden_device_state_t App_Garden_Get_State(void);
garden_sensor_data_t App_Garden_Get_Sensors(void);

void App_Garden_Set_Corridor_Light(garden_switch_state_t state);
void App_Garden_Set_Strip_Light(garden_switch_state_t state);
void App_Garden_Set_Veg_Light(garden_switch_state_t state);
void App_Garden_Set_Fan(garden_switch_state_t state);
void App_Garden_Set_Servo(garden_servo_state_t state);
void App_Garden_Set_Auto_Mode(bool enable);

void App_Garden_Set_Water_Pump(garden_switch_state_t state);
void App_Garden_Set_Atomizer(garden_switch_state_t state);
void App_Garden_Set_Safety_Switch(garden_switch_state_t state);

void App_Garden_All_Lights_On(void);
void App_Garden_All_Lights_Off(void);

void App_Garden_Fire_Emergency_Response(void);

void App_Garden_Get_Status_Json(char* buffer, int buffer_size);
void App_Garden_Get_Sensors_Json(char* buffer, int buffer_size);

bool App_Garden_Process_Mqtt_Cmd(const char* json_cmd, int cmd_len);

#ifdef __cplusplus
}
#endif

#endif
