/**
 * @file    sensor_adc_task.h
 * @brief   三楼从机统一传感器任务头文件
 */
#ifndef SENSOR_ADC_TASK_H
#define SENSOR_ADC_TASK_H

#include <stdint.h>

void Sensor_ADC_Task_Init(void);
void Sensor_ADC_Notify_Relay_Manual(uint8_t relay_index);

#endif
