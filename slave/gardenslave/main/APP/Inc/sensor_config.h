#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/adc.h"

#define SOIL_ADC_CHANNEL        ADC1_CHANNEL_0
#define RAIN_ADC_CHANNEL        ADC1_CHANNEL_3

#define SOIL_ADC_ATTEN          ADC_ATTEN_DB_12
#define RAIN_ADC_ATTEN          ADC_ATTEN_DB_12

#define ADC_SAMPLE_INTERVAL_MS  500

#define DEBUG_MODE              0

#define RAIN_THRESHOLD_PERCENT  60
#define SOIL_THRESHOLD_PERCENT  60
#define SOIL_WATER_START_PERCENT 45
#define SOIL_WATER_STOP_PERCENT  SOIL_THRESHOLD_PERCENT
#define WATER_PUMP_MAX_RUN_MS    0
#define WATER_PUMP_COOLDOWN_MS   0
#define RAIN_TRIGGER_COUNT      2

#define TEMP_COOLING_ON_C        32.0f
#define TEMP_COOLING_OFF_C       29.0f
#define HUMIDITY_DRY_ON_PERCENT  35.0f
#define HUMIDITY_DRY_OFF_PERCENT 55.0f
#define HUMIDITY_FAN_ON_PERCENT  80.0f
#define HUMIDITY_FAN_OFF_PERCENT 70.0f

#define FIRE_PIN                GPIO_NUM_2
#define HELP_PIN                GPIO_NUM_48
#define DHT_PIN                 GPIO_NUM_38

#endif
