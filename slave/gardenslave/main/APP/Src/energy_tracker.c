#include "energy_tracker.h"
#include "mqtt_receive.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "ENERGY";

static energy_accum_t g_energy = {0};
static bool g_last_on_state[ENERGY_DEV_COUNT] = {false};
static int64_t g_last_update_us = 0;
static int64_t g_last_save_us = 0;
static int64_t g_last_report_us = 0;

static const float RATED_WATTS[ENERGY_DEV_COUNT] = {
    ENERGY_RATE_W_CORRIDOR_LIGHT,
    ENERGY_RATE_W_STRIP_LIGHT,
    ENERGY_RATE_W_VEG_LIGHT,
    ENERGY_RATE_W_FAN,
    ENERGY_RATE_W_WATER_PUMP,
    ENERGY_RATE_W_ATOMIZER
};

static void NVS_Save(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ENERGY_NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_blob(handle, ENERGY_NVS_KEY, &g_energy, sizeof(g_energy));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed: %s", esp_err_to_name(err));
    } else {
        nvs_commit(handle);
        ESP_LOGI(TAG, "NVS saved");
    }

    nvs_close(handle);
}

static void NVS_Load(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(ENERGY_NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed, using defaults");
        return;
    }

    size_t sz = sizeof(g_energy);
    err = nvs_get_blob(handle, ENERGY_NVS_KEY, &g_energy, &sz);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS loaded: total=%.3f kWh", Energy_Tracker_Get_Total());
    } else {
        ESP_LOGW(TAG, "NVS read failed, using defaults");
        memset(&g_energy, 0, sizeof(g_energy));
    }

    nvs_close(handle);
}

void Energy_Tracker_Init(void) {
    memset(&g_energy, 0, sizeof(g_energy));
    NVS_Load();

    g_last_update_us = esp_timer_get_time();
    g_last_save_us = g_last_update_us;
    g_last_report_us = g_last_update_us;

    ESP_LOGI(TAG, "Energy tracker initialized");
}

void Energy_Tracker_Update(bool device_on[ENERGY_DEV_COUNT]) {
    int64_t now = esp_timer_get_time();
    int64_t delta_us = now - g_last_update_us;
    g_last_update_us = now;

    if (delta_us <= 0 || delta_us > 60000000) {
        return;
    }

    for (int i = 0; i < ENERGY_DEV_COUNT; i++) {
        g_last_on_state[i] = device_on[i];
        if (device_on[i]) {
            float delta_kwh = (RATED_WATTS[i] / 1000.0f) * ((float)(delta_us * ENERGY_DEMO_SPEEDUP) / 3600000000.0f);
            g_energy.kwh[i] += delta_kwh;
        }
    }

    if ((now - g_last_save_us) >= (int64_t)ENERGY_SAVE_INTERVAL_MS * 1000) {
        NVS_Save();
        g_last_save_us = now;
    }

    if ((now - g_last_report_us) >= (int64_t)ENERGY_REPORT_INTERVAL_S * 1000000) {
        Energy_Tracker_Report_Now();
        g_last_report_us = now;
    }
}

float Energy_Tracker_Get_Total(void) {
    float total = 0;
    for (int i = 0; i < ENERGY_DEV_COUNT; i++) {
        total += g_energy.kwh[i];
    }
    return total;
}

const energy_accum_t* Energy_Tracker_Get_Data(void) {
    return &g_energy;
}

void Energy_Tracker_Report_Now(void) {
    MQTT_Publish_Energy(&g_energy, g_last_on_state);
    ESP_LOGI(TAG, "Energy report sent: total=%.3f kWh", Energy_Tracker_Get_Total());
}
