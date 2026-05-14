#include "app_fall_detect.h"
#include "app_buzzer.h"
#include "app_mpu6050.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <math.h>

static const char *TAG = "FALL_DETECT";

static bool g_fall_detect_enabled = true;
static bool g_alarm_active = false;
static uint8_t g_high_count = 0;
static uint32_t g_last_fall_time = 0;

void App_Fall_Detect_Init(void) {
    ESP_LOGI(TAG, "Fall detection initialized");
    ESP_LOGI(TAG, "  Threshold: %.1fG (fall)", (double)FALL_DETECT_FALL_G);
}

void App_Fall_Detect_Update(float accel_x, float accel_y, float accel_z) {
    if (!g_fall_detect_enabled) {
        return;
    }

    float accel_mag = sqrtf(accel_x * accel_x + accel_y * accel_y + accel_z * accel_z);

    if (g_alarm_active) {
        uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000);
        if (current_time - g_last_fall_time > FALL_RECOVERY_TIME_MS) {
            g_alarm_active = false;
            g_high_count = 0;
            App_Buzzer_Off();
            ESP_LOGI(TAG, "Fall alarm auto-reset");
        }
        return;
    }

    if (accel_mag > FALL_DETECT_FALL_G || accel_mag < 0.3f) {
        g_high_count++;
        if (g_high_count >= FALL_CONFIRM_COUNT) {
            g_alarm_active = true;
            g_last_fall_time = (uint32_t)(esp_timer_get_time() / 1000);
            g_high_count = 0;
            ESP_LOGW(TAG, "FALL DETECTED! accel_mag=%.2fG", accel_mag);
            App_Buzzer_On();
        }
    } else {
        if (g_high_count > 0) {
            g_high_count--;
        }
    }
}

bool App_Fall_Detect_Is_Falling(void) {
    return g_alarm_active;
}

void App_Fall_Detect_Reset(void) {
    g_alarm_active = false;
    g_high_count = 0;
    App_Buzzer_Off();
    ESP_LOGI(TAG, "Fall detection reset");
}
