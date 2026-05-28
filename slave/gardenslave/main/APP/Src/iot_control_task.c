#include "iot_control_task.h"
#include "mqtt_receive.h"
#include "app_asr_uart.h"
#include "app_ws2812_strip.h"
#include "mqtt_iot_protocol.h"
#include "energy_tracker.h"

#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char* TAG = "IOT_CTRL";

TaskHandle_t s_iot_ctrl_task_handle = NULL;

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_14_BIT
#define LEDC_FREQUENCY          50
#define SERVO_MIN_PULSEWIDTH_US 500
#define SERVO_MAX_PULSEWIDTH_US 2500
#define SERVO_PWM_MAX_DEG        180

static ledc_channel_t g_servo_channels[SERVO_COUNT] = {LEDC_CHANNEL_0};

static void HW_Gpio_Init(gpio_num_t gpio) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&cfg);
}

static void HW_Gpio_Write(gpio_num_t gpio, uint8_t level) {
    gpio_set_level(gpio, level);
}

static void HW_Servo_Init(gpio_num_t gpio, ledc_channel_t channel) {
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER,
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t ch = {
        .speed_mode = LEDC_MODE,
        .channel = channel,
        .gpio_num = gpio,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch);

    ESP_LOGI(TAG, "Servo initialized on GPIO %d, channel %d", gpio, channel);
}

static void HW_Servo_Set_Angle(ledc_channel_t channel, int16_t angle) {
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
    if (angle < 0) angle = 0;

    float pulse_width = SERVO_MIN_PULSEWIDTH_US +
        (float)(angle) * (SERVO_MAX_PULSEWIDTH_US - SERVO_MIN_PULSEWIDTH_US) / SERVO_PWM_MAX_DEG;

    uint32_t duty = (uint32_t)((pulse_width * 16384) / 20000);
    ledc_set_duty(LEDC_MODE, channel, duty);
    ledc_update_duty(LEDC_MODE, channel);
}

#define LIGHT_CORRIDOR_GPIO  GPIO_NUM_11
#define LIGHT_STRIP_GPIO     GPIO_NUM_12
#define LIGHT_VEG_GPIO       GPIO_NUM_13
#define FAN_RELAY_GPIO       GPIO_NUM_14
#define WATER_PUMP_GPIO      GPIO_NUM_15
#define ATOMIZER_GPIO        GPIO_NUM_16
#define SAFETY_SWITCH_GPIO   GPIO_NUM_17
#define SERVO_RAIN_SHELTER_GPIO GPIO_NUM_18

#define FIRE_PIN             GPIO_NUM_2
#define HELP_PIN             GPIO_NUM_48
#define DHT_PIN              GPIO_NUM_38

#define REFRESH_INTERVAL_MS  100
#define LIGHT_STRIP_INDEX    1

static gpio_num_t g_light_gpios[LIGHT_COUNT] = {
    LIGHT_CORRIDOR_GPIO, LIGHT_STRIP_GPIO, LIGHT_VEG_GPIO
};

static gpio_num_t g_relay_gpios[RELAY_COUNT] = {
    FAN_RELAY_GPIO, WATER_PUMP_GPIO, ATOMIZER_GPIO, SAFETY_SWITCH_GPIO
};

static gpio_num_t g_servo_gpios[SERVO_COUNT] = {SERVO_RAIN_SHELTER_GPIO};

volatile device_flags_enum_t g_device_flags = {
    .light = {SWITCH_OFF, SWITCH_OFF, SWITCH_OFF},
    .relay = {SWITCH_OFF, SWITCH_OFF, SWITCH_OFF, SWITCH_ON},
    .servo = {SERVO_MIN_ANGLE},
    .fire_status = FIRE_NORMAL,
    .auto_mode = false,
    .auto_fan = false,
    .auto_atomizer = false,
    .auto_watering = false,
    .auto_rain_shelter = false,
    .strip_red = 255,
    .strip_green = 185,
    .strip_blue = 110,
    .strip_brightness = 35,
    .strip_effect = IOT_STRIP_EFFECT_SOLID,
    .ambience_scene = IOT_AMBIENCE_SCENE_NORMAL
};

SemaphoreHandle_t g_device_flags_mutex = NULL;

volatile bool g_help_detected = false;

bool Iot_Device_Flags_Lock(const char* caller) {
    if (g_device_flags_mutex == NULL) {
        ESP_LOGE(TAG, "Device flags mutex not ready: %s", caller ? caller : "unknown");
        return false;
    }

    if (xSemaphoreTake(g_device_flags_mutex, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to lock device flags: %s", caller ? caller : "unknown");
        return false;
    }

    return true;
}

void Iot_Device_Flags_Unlock(const char* caller) {
    if (g_device_flags_mutex == NULL) {
        ESP_LOGE(TAG, "Device flags mutex missing on unlock: %s", caller ? caller : "unknown");
        return;
    }

    xSemaphoreGive(g_device_flags_mutex);
}

static void IRAM_ATTR help_isr_handler(void* arg) {
    g_help_detected = true;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskNotifyFromISR(s_iot_ctrl_task_handle, NOTIFY_HELP_BIT, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void Init_Interrupt_Pins(void) {
    gpio_install_isr_service(0);

    gpio_config_t help_conf = {
        .pin_bit_mask = (1ULL << HELP_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&help_conf);
    gpio_isr_handler_add(HELP_PIN, help_isr_handler, NULL);

    ESP_LOGI(TAG, "Interrupt pins initialized: help=GPIO%d", HELP_PIN);
}

void Iot_Fire_Emergency_Response(void) {
    ESP_LOGW(TAG, "=== FIRE EMERGENCY RESPONSE (SLAVE) ===");

    bool first_fire_report = false;

    DEVICE_FLAGS_LOCK();
    first_fire_report = (g_device_flags.fire_status != FIRE_CONFIRMED);
    g_device_flags.fire_status = FIRE_CONFIRMED;
    g_device_flags.relay[3] = SWITCH_OFF;
    g_device_flags.relay[1] = SWITCH_OFF;
    g_device_flags.relay[2] = SWITCH_OFF;
    for (int i = 0; i < LIGHT_COUNT; i++) {
        g_device_flags.light[i] = SWITCH_ON;
    }
    g_device_flags.strip_red = 255;
    g_device_flags.strip_green = 0;
    g_device_flags.strip_blue = 0;
    g_device_flags.strip_brightness = 100;
    g_device_flags.strip_effect = IOT_STRIP_EFFECT_ALERT;
    g_device_flags.ambience_scene = IOT_AMBIENCE_SCENE_SAFE;
    DEVICE_FLAGS_UNLOCK();
    MQTT_Publish_Scene(IOT_AMBIENCE_SCENE_SAFE);

    if (first_fire_report) {
        MQTT_Publish_Emergency(EMERGENCY_FIRE, "Fire detected by smoke sensor");
        App_ASR_Send_Fire_Alert();
    } else {
        ESP_LOGW(TAG, "Fire already confirmed, local outputs refreshed without duplicate report");
    }

    ESP_LOGW(TAG, "Fire emergency: safety switch CUT OFF, pump/atomizer OFF, all lights ON");
}

void Iot_Help_Emergency_Response(void) {
    ESP_LOGW(TAG, "=== HELP EMERGENCY RESPONSE (SLAVE) ===");

    DEVICE_FLAGS_LOCK();
    for (int i = 0; i < LIGHT_COUNT; i++) {
        g_device_flags.light[i] = SWITCH_ON;
    }
    g_device_flags.strip_red = 255;
    g_device_flags.strip_green = 80;
    g_device_flags.strip_blue = 40;
    g_device_flags.strip_brightness = 100;
    g_device_flags.strip_effect = IOT_STRIP_EFFECT_ALERT;
    DEVICE_FLAGS_UNLOCK();

    MQTT_Publish_Emergency(EMERGENCY_HELP, "Help button pressed");

    App_ASR_Send_Help_Alert();

    ESP_LOGW(TAG, "Help emergency: all lights ON, MQTT alert sent");
}

void Iot_Rain_Alert_Response(void) {
    ESP_LOGW(TAG, "=== RAIN ALERT (SLAVE) ===");

    MQTT_Publish_Emergency(EMERGENCY_RAIN, "Rain detected, rain shelter auto-deployed");

    ESP_LOGW(TAG, "Rain alert: MQTT notification sent to master");
}

static void Device_Refresh_Task(void* arg) {
    ESP_LOGI(TAG, "Device refresh task started");

    for (int i = 0; i < LIGHT_COUNT; i++) {
        if (i == LIGHT_STRIP_INDEX) {
            App_WS2812_Strip_Init(LIGHT_STRIP_GPIO, WS2812_STRIP_LED_COUNT);
            ESP_LOGI(TAG, "Light[%d] -> GPIO%d (WS2812B strip)", i, g_light_gpios[i]);
            continue;
        }
        HW_Gpio_Init(g_light_gpios[i]);
        gpio_set_level(g_light_gpios[i], 0);
        ESP_LOGI(TAG, "Light[%d] -> GPIO%d", i, g_light_gpios[i]);
    }

    for (int i = 0; i < RELAY_COUNT; i++) {
        HW_Gpio_Init(g_relay_gpios[i]);
        gpio_set_level(g_relay_gpios[i], i == 3 ? 1 : 0);
        ESP_LOGI(TAG, "Relay[%d] -> GPIO%d (%s)", i, g_relay_gpios[i],
                 i == 0 ? "通风扇" : i == 1 ? "水泵" : i == 2 ? "雾化器" : "安防总闸");
    }

    for (int i = 0; i < SERVO_COUNT; i++) {
        HW_Servo_Init(g_servo_gpios[i], g_servo_channels[i]);
        HW_Servo_Set_Angle(g_servo_channels[i], 0);
        ESP_LOGI(TAG, "Servo[%d] -> GPIO%d", i, g_servo_gpios[i]);
    }

    Init_Interrupt_Pins();

    while (1) {
        device_flags_enum_t flags_snapshot;

        DEVICE_FLAGS_LOCK();
        flags_snapshot = g_device_flags;
        DEVICE_FLAGS_UNLOCK();

        for (int i = 0; i < LIGHT_COUNT; i++) {
            if (i == LIGHT_STRIP_INDEX) {
                App_WS2812_Strip_Apply(flags_snapshot.light[i] == SWITCH_ON,
                                       flags_snapshot.strip_red,
                                       flags_snapshot.strip_green,
                                       flags_snapshot.strip_blue,
                                       flags_snapshot.strip_brightness,
                                       flags_snapshot.strip_effect);
            } else {
                HW_Gpio_Write(g_light_gpios[i], flags_snapshot.light[i]);
            }
        }
        for (int i = 0; i < RELAY_COUNT; i++) {
            HW_Gpio_Write(g_relay_gpios[i], flags_snapshot.relay[i]);
        }
        for (int i = 0; i < SERVO_COUNT; i++) {
            HW_Servo_Set_Angle(g_servo_channels[i], flags_snapshot.servo[i]);
        }

        {
            bool energy_dev_on[ENERGY_DEV_COUNT] = {
                flags_snapshot.light[0] == SWITCH_ON,
                flags_snapshot.light[1] == SWITCH_ON,
                flags_snapshot.light[2] == SWITCH_ON,
                flags_snapshot.relay[0] == SWITCH_ON,
                flags_snapshot.relay[1] == SWITCH_ON,
                flags_snapshot.relay[2] == SWITCH_ON
            };
            Energy_Tracker_Update(energy_dev_on);
        }

        uint32_t notified_bits = 0;
        xTaskNotifyWait(0, NOTIFY_HELP_BIT | NOTIFY_RAIN_BIT, &notified_bits, 0);

        if (notified_bits & NOTIFY_HELP_BIT) {
            ESP_LOGW(TAG, "HELP button pressed!");
            Iot_Help_Emergency_Response();
        }

        if (notified_bits & NOTIFY_RAIN_BIT) {
            Iot_Rain_Alert_Response();
        }

        vTaskDelay(pdMS_TO_TICKS(REFRESH_INTERVAL_MS));
    }
}

void Iot_Control_Task_Init(void) {
    g_device_flags_mutex = xSemaphoreCreateMutex();
    if (g_device_flags_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create device flags mutex");
        return;
    }

    App_ASR_UART_Init();

    BaseType_t ret = xTaskCreate(
        Device_Refresh_Task,
        "iot_ctrl",
        4096,
        NULL,
        3,
        &s_iot_ctrl_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create device refresh task");
    } else {
        ESP_LOGI(TAG, "Device control initialized: %d lights, %d relays, %d servos",
                 LIGHT_COUNT, RELAY_COUNT, SERVO_COUNT);
    }
}
