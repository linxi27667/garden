#ifndef __BSP_GARDEN_H__
#define __BSP_GARDEN_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    GARDEN_SWITCH_OFF = 0,
    GARDEN_SWITCH_ON = 1
} garden_switch_state_t;

typedef enum {
    GARDEN_SERVO_COLLECT = 0,
    GARDEN_SERVO_HANG = 1,
    GARDEN_SERVO_RETRACT = GARDEN_SERVO_COLLECT,
    GARDEN_SERVO_DEPLOY = GARDEN_SERVO_HANG
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
    uint8_t soil_percent;
    uint16_t rain_adc_mv;
    uint8_t rain_percent;
    garden_rain_state_t rain_state;
    garden_fire_state_t fire_status;
    uint8_t help_status;
    float temperature;
    float humidity;
    uint32_t last_update_ms;
} garden_sensor_data_t;

typedef struct {
    garden_switch_state_t corridor_light;
    garden_switch_state_t strip_light;
    garden_switch_state_t veg_light;
    uint8_t veg_pwm_duty;
    uint8_t strip_red;
    uint8_t strip_green;
    uint8_t strip_blue;
    uint8_t strip_brightness;
    uint8_t strip_effect;
    uint8_t ambience_scene;
    garden_switch_state_t fan;
    garden_servo_state_t servo;
    garden_switch_state_t water_pump;
    garden_switch_state_t atomizer;
    garden_switch_state_t safety_switch;
    bool auto_mode;
    bool auto_fan;
    bool auto_atomizer;
    bool auto_watering;
    bool auto_rain_shelter;
    garden_fire_state_t fire_status;
} garden_device_state_t;

#endif
