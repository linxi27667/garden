#ifndef GARDEN_MODEL_H
#define GARDEN_MODEL_H

#include <stdbool.h>
#include <stdint.h>

#define WIFI_SSID_MAX 33
#define WIFI_SCAN_MAX 16

typedef enum {
    WIFI_STATE_IDLE,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED,
} wifi_state_t;

typedef struct {
    char ssid[WIFI_SSID_MAX];
    int8_t rssi;
    bool encrypted;
} wifi_ap_t;

typedef struct {
    bool corridor_light;
    bool indoor_strip;
    bool water_pump;
    bool mist_cooler;

    uint16_t light_sensor;
    uint16_t smoke_sensor;
    uint16_t rain_sensor;

    bool auto_mode;

    bool veg_light;
    uint8_t veg_pwm_duty;
    bool servo_hang;
    uint8_t soil_moisture;
    bool rain_wet;

    wifi_state_t wifi_state;
    char wifi_ssid[WIFI_SSID_MAX];
    wifi_ap_t wifi_aps[WIFI_SCAN_MAX];
    int wifi_ap_count;
} garden_model_t;

void garden_model_init(void);
void garden_model_start(void);
void garden_model_stop(void);
const garden_model_t *garden_model_get(void);

void garden_model_set_device(const char *device, bool on);
void garden_model_set_auto(bool on);
void garden_model_set_servo(bool hang);

void garden_model_wifi_scan(void);
void garden_model_wifi_connect(const char *ssid, const char *password);
void garden_model_wifi_disconnect(void);

#endif
