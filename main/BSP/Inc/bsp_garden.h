#ifndef __BSP_GARDEN_H__
#define __BSP_GARDEN_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GARDEN_LIGHT_OFF = 0,
    GARDEN_LIGHT_ON = 1
} garden_light_state_t;

typedef enum {
    GARDEN_SERVO_COLLECT = 0,
    GARDEN_SERVO_HANG = 1
} garden_servo_state_t;

typedef enum {
    GARDEN_RAIN_DRY = 0,
    GARDEN_RAIN_WET = 1
} garden_rain_state_t;

typedef enum {
    GARDEN_FIRE_NORMAL = 0,
    GARDEN_FIRE_WARNING = 1,
    GARDEN_FIRE_CONFIRMED = 2
} garden_fire_state_t;

typedef struct {
    uint8_t soil_value;
    uint8_t light_value;
    garden_rain_state_t rain_state;
    garden_fire_state_t fire_status;
    uint8_t help_status;
    float temperature;
    float humidity;
    uint32_t last_update_ms;
} garden_sensor_data_t;

typedef struct {
    garden_light_state_t corridor_light;
    garden_light_state_t strip_light;
    garden_light_state_t veg_light;
    uint8_t veg_pwm_duty;
    garden_light_state_t fan;
    garden_servo_state_t servo;
    garden_light_state_t water_pump;
    garden_light_state_t atomizer;
    garden_light_state_t safety_switch;
    bool auto_mode;
    garden_fire_state_t fire_status;
} garden_device_state_t;

#endif
