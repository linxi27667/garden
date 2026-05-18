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

#define RAIN_THRESHOLD_MV       2000
#define RAIN_TRIGGER_COUNT      2

#define FIRE_PIN                GPIO_NUM_2
#define HELP_PIN                GPIO_NUM_48
#define DHT_PIN                 GPIO_NUM_38

#endif
