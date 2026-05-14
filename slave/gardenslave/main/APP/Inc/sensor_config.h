#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <stdint.h>
#include "driver/gpio.h"
#include "driver/adc.h"

#define SOIL_ADC_CHANNEL        ADC1_CHANNEL_0
#define LIGHT_ADC_CHANNEL       ADC1_CHANNEL_2

#define SOIL_ADC_ATTEN          ADC_ATTEN_DB_12
#define LIGHT_ADC_ATTEN         ADC_ATTEN_DB_12

#define ADC_SAMPLE_INTERVAL_MS  500

#define DEBUG_MODE              0

#define RAIN_THRESHOLD_LIGHT    2640
#define RAIN_TRIGGER_COUNT      2

#define FIRE_PIN                GPIO_NUM_10
#define HELP_PIN                GPIO_NUM_11
#define DHT_PIN                 GPIO_NUM_8

#endif
