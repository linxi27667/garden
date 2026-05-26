#include "iot_mcp_tool.h"
#include "iot_controller.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "IOT_MCP";

bool Iot_Mcp_Init(void) {
    ESP_LOGI(TAG, "Initializing IOT MCP tool...");
    App_IOT_System_Init();
    ESP_LOGI(TAG, "IOT MCP tool initialized");
    return true;
}

bool Iot_Mcp_Set_GPIO(const char* device_name, const char* gpio_name, bool value) {
    ESP_LOGI(TAG, "Set GPIO: device=%s, gpio=%s, value=%d", 
             device_name ? device_name : "local", gpio_name, value);
    return App_IOT_Set_GPIO(device_name, gpio_name, value ? 1 : 0);
}

int Iot_Mcp_Get_GPIO(const char* device_name, const char* gpio_name) {
    return App_IOT_Get_GPIO(device_name, gpio_name);
}

void Iot_Mcp_Get_Status_JSON(char* buffer, int buffer_size) {
    App_IOT_Get_All_Status(buffer, buffer_size);
}

int Iot_Mcp_Get_Local_GPIO_Count(void) {
    return App_IOT_Get_Local_GPIO_Count();
}

const char* Iot_Mcp_Get_Local_GPIO_Name(int index) {
    return App_IOT_Get_Local_GPIO_Name(index);
}

int Iot_Mcp_Get_Remote_Device_Count(void) {
    return App_IOT_Get_Device_Count();
}

const char* Iot_Mcp_Get_Remote_Device_Name(int index) {
    return App_IOT_Get_Device_Name(index);
}

bool Iot_Mcp_Add_Remote_Device(const char* name, const uint8_t* mac, int gpio_count) {
    return App_IOT_Add_Remote_Device(name, mac, gpio_count);
}

void Iot_Mcp_Discover(void) {
    ESP_LOGI(TAG, "Starting device discovery...");
    App_IOT_Discover_Devices();
}

int Iot_Mcp_Wait_For_Response(int timeout_ms) {
    int initial_count = App_IOT_Get_Device_Count();
    int elapsed = 0;
    int check_interval = 100;
    
    while (elapsed < timeout_ms) {
        vTaskDelay(pdMS_TO_TICKS(check_interval));
        elapsed += check_interval;
        
        int current_count = App_IOT_Get_Device_Count();
        if (current_count > initial_count) {
            ESP_LOGI(TAG, "Discovered %d new device(s)", current_count - initial_count);
            return current_count - initial_count;
        }
    }
    
    return 0;
}

bool Iot_Mcp_Set_Servo(const char* device_name, uint8_t angle) {
    ESP_LOGI(TAG, "Set Servo: device=%s, angle=%d",
             device_name ? device_name : "local", angle);
    return App_IOT_Set_Servo(device_name, "servo", angle);
}

bool Iot_Mcp_Scenario_Homecoming(void) {
    ESP_LOGI(TAG, "[MCP] Scenario: Homecoming - opening door, turning on lights");
    return App_IOT_Scenario_Homecoming("一楼设备", "all");
}

bool Iot_Mcp_Scenario_LeaveHome(void) {
    ESP_LOGI(TAG, "[MCP] Scenario: LeaveHome - closing door");
    return App_IOT_Scenario_LeaveHome("一楼设备");
}

bool Iot_Mcp_Scenario_GoodNight(void) {
    ESP_LOGI(TAG, "[MCP] Scenario: GoodNight - turning off all lights");
    return App_IOT_Scenario_GoodNight("all");
}

bool Iot_Mcp_Scenario_Emergency(void) {
    ESP_LOGW(TAG, "[MCP] EMERGENCY triggered!");
    return App_IOT_Scenario_Emergency();
}

bool Iot_Mcp_Control_Servo_Angle(const char* device_name, uint8_t servo_index, uint8_t angle) {
    ESP_LOGI(TAG, "[MCP] Set Servo[%d] = %d deg on %s", servo_index, angle, device_name);
    return App_IOT_Control_Servo_Angle(device_name, servo_index, angle);
}

bool Iot_Mcp_Scenario_Rain_Collect_Clothes(void) {
    ESP_LOGI(TAG, "[MCP] Scenario: Rain - deploying shelter");
    return App_IOT_Scenario_Rain_Deploy_Shelter("三楼设备");
}

bool Iot_Mcp_Scenario_Ventilate(uint8_t window_angle, bool fan_on) {
    ESP_LOGI(TAG, "[MCP] Scenario: Ventilate - windows=%d, fan=%s",
             window_angle, fan_on ? "ON" : "OFF");
    return App_IOT_Scenario_Ventilate("三楼设备", window_angle, fan_on ? 1 : 0);
}

void Iot_Mcp_Get_Sensors_Json(char* buffer, int buffer_size) {
    App_IOT_Get_Sensors_Json(buffer, buffer_size);
}

void Iot_Mcp_Get_Event_Log_Json(char* buffer, int buffer_size) {
    App_IOT_Get_Event_Log_Json(buffer, buffer_size);
}
