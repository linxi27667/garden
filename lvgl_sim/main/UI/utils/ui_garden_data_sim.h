#ifndef UI_GARDEN_DATA_SIM_H
#define UI_GARDEN_DATA_SIM_H

#include "lvgl/lvgl.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool corridor_light;
    bool strip_light;
    bool veg_light;
    uint8_t veg_pwm_duty;
    bool fan;
    bool servo_hang;
    bool auto_mode;
    uint8_t soil;
    uint8_t light;
    bool rain_wet;
} ui_garden_state_t;

void ui_garden_data_sim_init(void);
void ui_garden_data_sim_start(void);
void ui_garden_data_sim_stop(void);
ui_garden_state_t *ui_garden_data_sim_get(void);

void ui_garden_data_sim_set_light(const char *device, bool on);
void ui_garden_data_sim_set_fan(bool on);
void ui_garden_data_sim_set_servo(bool hang);
void ui_garden_data_sim_set_auto(bool on);

#endif
