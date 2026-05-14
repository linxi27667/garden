#include "ui_garden_data_sim.h"
#include <stdlib.h>
#include <string.h>

static ui_garden_state_t s_state;
static lv_timer_t *s_timer;

static void sim_tick(lv_timer_t *timer)
{
    (void)timer;
    s_state.soil = 30 + (rand() % 20);
    s_state.light = 40 + (rand() % 30);
    if (s_state.auto_mode) {
        if (s_state.veg_light) {
            if (s_state.light < 20) s_state.veg_pwm_duty = 100;
            else if (s_state.light > 70) s_state.veg_pwm_duty = 10;
            else s_state.veg_pwm_duty = (uint8_t)(100 - s_state.light);
        }
        if (s_state.rain_wet && s_state.servo_hang) {
            s_state.servo_hang = false;
        }
    }
}

void ui_garden_data_sim_init(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_state.auto_mode = true;
    s_state.servo_hang = true;
    s_state.soil = 45;
    s_state.light = 55;
}

void ui_garden_data_sim_start(void)
{
    if (!s_timer) {
        s_timer = lv_timer_create(sim_tick, 2000, NULL);
    }
}

void ui_garden_data_sim_stop(void)
{
    if (s_timer) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
}

ui_garden_state_t *ui_garden_data_sim_get(void)
{
    return &s_state;
}

void ui_garden_data_sim_set_light(const char *device, bool on)
{
    if (strcmp(device, "corridor_light") == 0) s_state.corridor_light = on;
    else if (strcmp(device, "strip_light") == 0) s_state.strip_light = on;
    else if (strcmp(device, "veg_light") == 0) {
        s_state.veg_light = on;
        s_state.veg_pwm_duty = on ? 80 : 0;
    }
}

void ui_garden_data_sim_set_fan(bool on)
{
    s_state.fan = on;
}

void ui_garden_data_sim_set_servo(bool hang)
{
    s_state.servo_hang = hang;
}

void ui_garden_data_sim_set_auto(bool on)
{
    s_state.auto_mode = on;
}
