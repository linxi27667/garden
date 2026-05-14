#include "bsp_mpu6050.h"
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char *TAG = "BSP_MPU6050";

static SemaphoreHandle_t i2c_mutex = NULL;

void MPU6050_Set_I2C_Mutex(void* mutex) {
    i2c_mutex = (SemaphoreHandle_t)mutex;
}

#ifdef CONFIG_USE_SW_I2C_FOR_MPU6050

static esp_err_t MPU6050_Write_Reg(mpu_t *mpu, uint8_t reg, uint8_t data) {
    if (mpu->write == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (i2c_mutex) {
        xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100));
    }
    esp_err_t ret = mpu->write(mpu->dev_addr, reg, data);
    if (i2c_mutex) {
        xSemaphoreGive(i2c_mutex);
    }
    return ret;
}

static esp_err_t MPU6050_Read_Regs(mpu_t *mpu, uint8_t reg, uint8_t *data, uint16_t len) {
    if (mpu->read == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (i2c_mutex) {
        xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100));
    }
    esp_err_t ret = mpu->read(mpu->dev_addr, reg, data, len);
    if (i2c_mutex) {
        xSemaphoreGive(i2c_mutex);
    }
    return ret;
}

#else

static esp_err_t MPU6050_Write_Reg(mpu_t *mpu, uint8_t reg, uint8_t data) {
    if (i2c_mutex) {
        xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100));
    }
    uint8_t write_buf[2] = {reg, data};
    esp_err_t ret = i2c_master_transmit(mpu->i2c_dev, write_buf, sizeof(write_buf), 100);
    if (i2c_mutex) {
        xSemaphoreGive(i2c_mutex);
    }
    return ret;
}

static esp_err_t MPU6050_Read_Regs(mpu_t *mpu, uint8_t reg, uint8_t *data, uint16_t len) {
    if (i2c_mutex) {
        xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100));
    }
    esp_err_t ret = i2c_master_transmit_receive(mpu->i2c_dev, &reg, 1, data, len, 100);
    if (i2c_mutex) {
        xSemaphoreGive(i2c_mutex);
    }
    return ret;
}

#endif

uint8_t MPU6050_Init_Device(mpu_t *mpu) {
    if (mpu == NULL || mpu->i2c_dev == NULL) {
        ESP_LOGE(TAG, "Invalid MPU6050 device handle");
        return 1;
    }

    uint8_t check_id = 0;
    uint8_t write_data = 0;

    esp_err_t ret = MPU6050_Read_Regs(mpu, MPU_WHO_AM_I_REG, &check_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WHO_AM_I register: %s", esp_err_to_name(ret));
        return 1;
    }

    if (check_id != 0x68 && check_id != 0x71 && check_id != 0x70 && check_id != 0x73) {
        ESP_LOGE(TAG, "Invalid WHO_AM_I value: 0x%02X (expected 0x68, 0x70, 0x71 or 0x73)", check_id);
        return 1;
    }

    const char* chip_name = "Unknown";
    if (check_id == 0x68) chip_name = "MPU6050";
    else if (check_id == 0x70) chip_name = "MPU6500/MPU9250";
    else if (check_id == 0x71) chip_name = "MPU6050 (alt)";
    else if (check_id == 0x73) chip_name = "MPU9255";
    
    ESP_LOGI(TAG, "%s detected, WHO_AM_I: 0x%02X", chip_name, check_id);

    write_data = 0x00;
    ret = MPU6050_Write_Reg(mpu, MPU_PWR_MGMT_1_REG, write_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wake up MPU6050");
        return 1;
    }

    if (mpu->Delay_ms) {
        mpu->Delay_ms(50);
    } else {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    write_data = 0x07;
    ret = MPU6050_Write_Reg(mpu, MPU_SMPLRT_DIV_REG, write_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set sample rate divider");
        return 1;
    }

    write_data = 0x06;
    ret = MPU6050_Write_Reg(mpu, MPU_CONFIG_REG, write_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set config register");
        return 1;
    }

    write_data = 0x00;
    ret = MPU6050_Write_Reg(mpu, MPU_ACCEL_CONFIG_REG, write_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set accel config");
        return 1;
    }

    write_data = 0x18;
    ret = MPU6050_Write_Reg(mpu, MPU_GYRO_CONFIG_REG, write_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set gyro config");
        return 1;
    }

    ESP_LOGI(TAG, "MPU6050 initialized successfully");
    return 0;
}

void MPU6050_Read_Data(mpu_t *mpu) {
    if (mpu == NULL || mpu->i2c_dev == NULL) {
        return;
    }

    uint8_t raw[14];

    esp_err_t ret = MPU6050_Read_Regs(mpu, MPU_ACCEL_XOUT_H_REG, raw, 14);
    if (ret != ESP_OK) {
        return;
    }

    int16_t ax = (raw[0] << 8) | raw[1];
    int16_t ay = (raw[2] << 8) | raw[3];
    int16_t az = (raw[4] << 8) | raw[5];
    int16_t temp = (raw[6] << 8) | raw[7];
    int16_t gx = (raw[8] << 8) | raw[9];
    int16_t gy = (raw[10] << 8) | raw[11];
    int16_t gz = (raw[12] << 8) | raw[13];

    mpu->accel_x = (float)ax / 16384.0f;
    mpu->accel_y = (float)ay / 16384.0f;
    mpu->accel_z = (float)az / 16384.0f;

    mpu->temp = 36.53f + ((float)temp / 340.0f);

    mpu->gyro_x = (float)gx / 16.4f;
    mpu->gyro_y = (float)gy / 16.4f;
    mpu->gyro_z = (float)gz / 16.4f;
}
