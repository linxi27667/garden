#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/oled_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "led/single_led.h"
#include "mcp_server.h"
#include "iot_mcp_tool.h"
#include "alarm_mcp_tool.h"
#include "garden_mcp_tool.h"
#include "app_mpu6050.h"
#include "app_buzzer.h"
#include "app_fall_detect.h"
#include "wifi_manager.h"
#include "bsp_mpu6050.h"
#include "iot_controller.h"
#include "app_garden.h"

#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>

#define TAG "CustomEsp32s3Cam"

#ifndef ENABLE_LEGACY_IOT_MCP_TOOLS
#define ENABLE_LEGACY_IOT_MCP_TOOLS 0
#endif

struct SensorAlertScenario {
    const char* type;
    const char* status;
    const char* emotion;
    const char* text;
    int beep_count;
    int beep_ms;
    int gap_ms;
};

static TaskHandle_t s_sensor_alert_task_handle = nullptr;
static const SensorAlertScenario* s_active_sensor_alert = nullptr;

static const SensorAlertScenario kSensorAlertScenarios[] = {
    {
        "fire",
        "火灾警报",
        "emergency",
        "检测到庭院火灾风险，系统已切断安防总闸，并停止水泵和雾化器。请立即确认现场安全。",
        15,
        500,
        200,
    },
    {
        "help",
        "紧急求助",
        "alert",
        "收到庭院求助信号，我已开启庭院照明，请尽快查看现场情况。",
        10,
        300,
        300,
    },
    {
        "rain",
        "雨天提醒",
        "rain",
        "检测到庭院降雨，已自动收回晾衣架并更新庭院状态。",
        3,
        180,
        500,
    },
};

static const SensorAlertScenario* FindSensorAlertScenario(const char* type) {
    if (type == nullptr) {
        return nullptr;
    }
    for (const auto& scenario : kSensorAlertScenarios) {
        if (strcmp(type, scenario.type) == 0) {
            return &scenario;
        }
    }
    return nullptr;
}

static void SensorAlertTask(void* arg) {
    const auto* scenario = static_cast<const SensorAlertScenario*>(arg);
    if (scenario == nullptr) {
        vTaskDelete(nullptr);
        return;
    }

    auto& app = Application::GetInstance();
    app.Schedule([scenario]() {
#if ENABLE_SENSOR_TTS_ALERT
        bool tts_sent = Application::GetInstance().SayText(scenario->text, scenario->status, scenario->emotion);
        if (tts_sent) {
            ESP_LOGI(TAG, "Sensor alert TTS sent: type=%s", scenario->type);
        } else {
            ESP_LOGW(TAG, "Sensor alert TTS failed, fallback to display alert: type=%s", scenario->type);
            Application::GetInstance().Alert(scenario->status, scenario->text, scenario->emotion);
        }
#else
        ESP_LOGW(TAG, "Sensor alert TTS disabled, using display alert only");
        Application::GetInstance().Alert(scenario->status, scenario->text, scenario->emotion);
#endif
    });

    for (int i = 0; i < scenario->beep_count; i++) {
        App_Buzzer_Beep(scenario->beep_ms);
        vTaskDelay(pdMS_TO_TICKS(scenario->gap_ms));
    }

    s_sensor_alert_task_handle = nullptr;
    s_active_sensor_alert = nullptr;
    vTaskDelete(nullptr);
}

static void DispatchSensorAlert(const char* type, const char* detail) {
    const SensorAlertScenario* scenario = FindSensorAlertScenario(type);
    if (scenario == nullptr) {
        ESP_LOGW(TAG, "Unknown sensor alert type=%s, detail=%s",
                 type ? type : "null", detail ? detail : "");
        return;
    }

    if (s_sensor_alert_task_handle != nullptr) {
        if (strcmp(scenario->type, "fire") == 0 &&
            (s_active_sensor_alert == nullptr || strcmp(s_active_sensor_alert->type, "fire") != 0)) {
            ESP_LOGW(TAG, "Fire alert preempts active sensor alert");
            vTaskDelete(s_sensor_alert_task_handle);
            App_Buzzer_Off();
            s_sensor_alert_task_handle = nullptr;
            s_active_sensor_alert = nullptr;
        } else {
        ESP_LOGW(TAG, "Sensor alert already running, skip duplicate type=%s", type);
        return;
        }
    }

    BaseType_t ret = xTaskCreate(SensorAlertTask, "sensor_alert", 3072,
                                 const_cast<SensorAlertScenario*>(scenario), 4,
                                 &s_sensor_alert_task_handle);
    if (ret != pdPASS) {
        s_sensor_alert_task_handle = nullptr;
        s_active_sensor_alert = nullptr;
        ESP_LOGE(TAG, "Failed to create sensor alert task, free_heap=%lu",
                 (unsigned long)esp_get_free_heap_size());
    } else {
        s_active_sensor_alert = scenario;
    }
}

class CustomEsp32s3Cam : public WifiBoard {
private:
    i2c_master_bus_handle_t display_i2c_bus_;
    Button boot_button_;
    Display* display_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    TaskHandle_t mpu6050_task_handle_ = nullptr;

    void InitializeDisplayI2c() {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
        ESP_LOGI(TAG, "Display I2C bus initialized on SDA=%d, SCL=%d", DISPLAY_SDA_PIN, DISPLAY_SCL_PIN);
    }

    void InitializeSsd1306Display() {
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = 0x3C,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
            .scl_speed_hz = 400 * 1000,
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(display_i2c_bus_, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Install SSD1306 driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = -1;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
        ESP_LOGI(TAG, "SSD1306 driver installed");

        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        if (esp_lcd_panel_init(panel_) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize display");
            display_ = new NoDisplay();
            return;
        }

        ESP_LOGI(TAG, "Turning display on");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        display_ = new OledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y);
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    void InitializeMpu6050() {
#ifdef CONFIG_USE_SW_I2C_FOR_MPU6050
        App_MPU6050_Init();
        ESP_LOGI(TAG, "MPU6050 initialized via software I2C");
#else
        App_MPU6050_Init(display_i2c_bus_);
        ESP_LOGI(TAG, "MPU6050 initialized on shared I2C bus");
#endif
    }

    void InitializeBuzzer() {
        App_Buzzer_Init(BUZZER_GPIO_PIN, BUZZER_ACTIVE_LEVEL);
        ESP_LOGI(TAG, "Buzzer initialized on GPIO %d", BUZZER_GPIO_PIN);
    }

    static void Mpu6050Task(void* arg) {
        ESP_LOGI(TAG, "MPU6050 task started");
        vTaskDelay(pdMS_TO_TICKS(1000));
        while (1) {
            App_MPU6050_Read();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

    static void AlarmTask(void* arg) {
        ESP_LOGI(TAG, "Alarm task started");
        vTaskDelay(pdMS_TO_TICKS(2000));
        while (1) {
            Alarm_Mcp_Process();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    void StartMpu6050Task() {
        xTaskCreatePinnedToCore(
            Mpu6050Task,
            "mpu6050_task",
            4096,
            NULL,
            2,
            &mpu6050_task_handle_,
            1
        );
    }

    void StartAlarmTask() {
        xTaskCreatePinnedToCore(
            AlarmTask,
            "alarm_task",
            4096,
            NULL,
            2,
            NULL,
            1
        );
        ESP_LOGI(TAG, "Alarm task started");
    }

public:
    CustomEsp32s3Cam() :
        boot_button_(BOOT_BUTTON_GPIO) {
        InitializeDisplayI2c();
        InitializeSsd1306Display();
        InitializeButtons();
        InitializeMpu6050();
        InitializeBuzzer();
        StartMpu6050Task();
        InitializeTools();
    }

    void StartNetwork() override {
        // First initialize WiFi (parent class)
        WifiBoard::StartNetwork();
        
        // Create a task to wait for WiFi connection and initialize ESP-NOW
        xTaskCreate([](void* arg) {
            auto* board = static_cast<CustomEsp32s3Cam*>(arg);
            
            // Wait for WiFi connection (check every 100ms)
            auto& wifi = WifiManager::GetInstance();
            while (!wifi.IsConnected()) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            
            // Wait a bit more for DHCP to complete
            vTaskDelay(pdMS_TO_TICKS(500));
            
            ESP_LOGI(TAG, "WiFi connected, initializing IoT controller...");
            board->InitializeIot();

            vTaskDelay(pdMS_TO_TICKS(2000));
            App_Garden_Start();

            vTaskDelete(NULL);
        }, "iot_init", 4096, this, 2, NULL);
    }

    void InitializeIot() {
        if (Iot_Mcp_Init()) {
            ESP_LOGI(TAG, "IOT controller initialized");
        } else {
            ESP_LOGW(TAG, "IOT controller initialization failed");
        }

        App_Garden_Init();

        App_IOT_Register_Emergency_Alert_Callback([](const char* type, const char* detail) {
            ESP_LOGW(TAG, "Emergency alert callback: type=%s, detail=%s", type, detail);
            DispatchSensorAlert(type, detail);
        });

        if (Alarm_Mcp_Init()) {
            ESP_LOGI(TAG, "Alarm manager initialized");
            
            Alarm_Mcp_Set_Callback([](alarm_info_t* alarm) {
                ESP_LOGI(TAG, "Alarm callback triggered: id=%d, message=%s", alarm->id, alarm->message);
                Application::GetInstance().Alert("闹钟提醒", alarm->message, "alert", "");
                for (int i = 0; i < 10; i++) {
                    App_Buzzer_Beep(200);
                    vTaskDelay(pdMS_TO_TICKS(300));
                }
            });
            
            StartAlarmTask();
        } else {
            ESP_LOGW(TAG, "Alarm manager initialization failed");
        }
    }

    void InitializeTools() {
        auto& mcp_server = McpServer::GetInstance();

#if ENABLE_LEGACY_IOT_MCP_TOOLS
        mcp_server.AddTool("self.iot.set_gpio",
            "控制远程 IoT 从机设备的 GPIO 输出。\n"
            "通过 MQTT 协议向指定从机发送 GPIO 控制指令。\n"
            "Args:\n"
            "  device: 从机设备名（如'智慧庭院'）\n"
            "  gpio: GPIO 名称\n"
            "  value: true=开/false=关\n"
            "当前系统仅有一个从机'智慧庭院'，该从机GPIO见 self.garden.* 系列工具。",
            PropertyList({
                Property("device", kPropertyTypeString),
                Property("gpio", kPropertyTypeString),
                Property("value", kPropertyTypeBoolean)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto device = properties["device"].value<std::string>();
                auto gpio = properties["gpio"].value<std::string>();
                auto value = properties["value"].value<bool>();
                
                bool success = Iot_Mcp_Set_GPIO(device.c_str(), gpio.c_str(), value);
                
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "device", device.c_str());
                cJSON_AddStringToObject(json, "gpio", gpio.c_str());
                cJSON_AddBoolToObject(json, "value", value);
                cJSON_AddStringToObject(json, "message", success ? 
                    (value ? "已打开" : "已关闭") : "操作失败");
                
                return json;
            });

        mcp_server.AddTool("self.iot.get_status",
            "Get the status of all IoT devices and GPIOs.\n"
            "Returns a list of local GPIOs and remote devices with their current states.",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                char buffer[4096];
                Iot_Mcp_Get_Status_JSON(buffer, sizeof(buffer));
                
                cJSON* json = cJSON_Parse(buffer);
                if (json == nullptr) {
                    json = cJSON_CreateObject();
                    cJSON_AddStringToObject(json, "raw", buffer);
                }
                
                return json;
            });

        mcp_server.AddTool("self.iot.get_sensors",
            "获取智慧庭院从机的实时传感器数据。\n"
            "返回土壤湿度(mV/%)、雨滴电压(mV)、雨滴状态(干燥/下雨)、火灾状态、求助状态、温度(°C)、湿度(%)。\n"
            "注意：光照传感器已移除，请勿询问光照数据。传感器详情见 self.garden.get_sensors。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                char buffer[1024];
                Iot_Mcp_Get_Sensors_Json(buffer, sizeof(buffer));

                cJSON* json = cJSON_Parse(buffer);
                if (json == nullptr) {
                    json = cJSON_CreateObject();
                    cJSON_AddStringToObject(json, "raw", buffer);
                }

                return json;
            });

        mcp_server.AddTool("self.iot.get_history",
            "获取最近的设备事件历史记录（最多50条）。\n"
            "包括设备上下线、命令执行、传感器状态变化等事件。\n"
            "当用户问'最近发生了什么'、'设备有什么变化'时使用。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                char buffer[4096];
                Iot_Mcp_Get_Event_Log_Json(buffer, sizeof(buffer));

                cJSON* json = cJSON_Parse(buffer);
                if (json == nullptr) {
                    json = cJSON_CreateObject();
                    cJSON_AddStringToObject(json, "raw", buffer);
                }

                return json;
            });

        mcp_server.AddTool("self.iot.discover",
            "Discover remote ESP32 IoT devices on the network.\n"
            "Sends a broadcast message to find available remote devices.",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                ESP_LOGI(TAG, "Starting device discovery via MCP tool...");
                
                Iot_Mcp_Discover();
                
                cJSON* json = cJSON_CreateObject();
                
                int total_count = Iot_Mcp_Get_Remote_Device_Count();
                
                if (total_count > 0) {
                    cJSON_AddStringToObject(json, "message", "发现命令已发送，下面是当前已知设备");
                } else {
                    cJSON_AddStringToObject(json, "message", "发现命令已发送，等待从机 announce 或 heartbeat 更新");
                }
                
                cJSON_AddNumberToObject(json, "remote_device_count", total_count);
                
                cJSON* devices = cJSON_AddArrayToObject(json, "known_devices");
                for (int i = 0; i < total_count; i++) {
                    const char* name = Iot_Mcp_Get_Remote_Device_Name(i);
                    if (name) {
                        cJSON_AddItemToArray(devices, cJSON_CreateString(name));
                    }
                }
                
                return json;
            });

        mcp_server.AddTool("self.mpu6050.get_data",
            "Get the current motion data from the MPU6050 sensor.\n"
            "Returns accelerometer data (g), gyroscope data (°/s), and temperature (°C).\n"
            "Use this tool when the user asks about motion, tilt, acceleration, or temperature.",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                cJSON* json = cJSON_CreateObject();
                
                cJSON* accel = cJSON_AddObjectToObject(json, "accelerometer");
                cJSON_AddNumberToObject(accel, "x", my_mpu.accel_x);
                cJSON_AddNumberToObject(accel, "y", my_mpu.accel_y);
                cJSON_AddNumberToObject(accel, "z", my_mpu.accel_z);
                
                cJSON* gyro = cJSON_AddObjectToObject(json, "gyroscope");
                cJSON_AddNumberToObject(gyro, "x", my_mpu.gyro_x);
                cJSON_AddNumberToObject(gyro, "y", my_mpu.gyro_y);
                cJSON_AddNumberToObject(gyro, "z", my_mpu.gyro_z);
                
                cJSON_AddNumberToObject(json, "temperature", my_mpu.temp);
                
                return json;
            });

        mcp_server.AddTool("self.buzzer.beep",
            "Control the buzzer to make sounds.\n"
            "Use this tool when the user wants to make a beep sound or alert.\n"
            "Args:\n"
            "  times: Number of beeps (1-10)\n"
            "  duration_ms: Duration of each beep in milliseconds (default 100)",
            PropertyList({
                Property("times", kPropertyTypeInteger),
                Property("duration_ms", kPropertyTypeInteger)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                int times = properties["times"].value<int>();
                int duration_ms = properties["duration_ms"].value<int>();
                
                if (times < 1) times = 1;
                if (times > 10) times = 10;
                if (duration_ms < 10) duration_ms = 100;
                if (duration_ms > 2000) duration_ms = 2000;
                
                App_Buzzer_Beep_Times(times, duration_ms, duration_ms);
                
                cJSON* json = cJSON_CreateObject();
                cJSON_AddNumberToObject(json, "times", times);
                cJSON_AddNumberToObject(json, "duration_ms", duration_ms);
                cJSON_AddStringToObject(json, "message", "蜂鸣器已响铃");
                
                return json;
            });

        mcp_server.AddTool("self.iot.set_light",
            "控制指定从机设备的灯光开关。\n"
            "Args:\n"
            "  device: 从机设备名（如'智慧庭院'）\n"
            "  index: 灯编号(0-2)\n"
            "  value: true=开/false=关\n"
            "推荐使用 self.garden.set_light 代替本工具以获取更精确的灯光名称映射。",
            PropertyList({
                Property("device", kPropertyTypeString),
                Property("index", kPropertyTypeInteger),
                Property("value", kPropertyTypeBoolean)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto device = properties["device"].value<std::string>();
                int index = properties["index"].value<int>();
                bool value = properties["value"].value<bool>();

                bool success = App_IOT_Set_Light(device.c_str(), (uint8_t)index, value);

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "device", device.c_str());
                cJSON_AddNumberToObject(json, "light", index);
                cJSON_AddBoolToObject(json, "value", value);
                cJSON_AddStringToObject(json, "message", success ?
                    (value ? "灯已打开" : "灯已关闭") : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.set_relay",
            "控制从机设备继电器开关。\n"
            "Args:\n"
            "  device: 从机设备名（如'智慧庭院'）\n"
            "  index: 继电器编号(0-3)\n"
            "  value: true=开/false=关\n"
            "推荐使用 self.garden.set_fan / set_water_pump / set_atomizer 代替，无需记忆继电器编号。",
            PropertyList({
                Property("device", kPropertyTypeString),
                Property("index", kPropertyTypeInteger),
                Property("value", kPropertyTypeBoolean)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto device = properties["device"].value<std::string>();
                int index = properties["index"].value<int>();
                bool value = properties["value"].value<bool>();

                bool success = App_IOT_Set_Relay(device.c_str(), (uint8_t)index, value);

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "device", device.c_str());
                cJSON_AddNumberToObject(json, "relay", index);
                cJSON_AddBoolToObject(json, "value", value);
                cJSON_AddStringToObject(json, "message", success ?
                    (value ? "继电器已打开" : "继电器已关闭") : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.set_servo_by_index",
            "控制从机设备舵机角度。\n"
            "Args:\n"
            "  device: 从机设备名（如'智慧庭院'）\n"
            "  index: 舵机编号\n"
            "  angle: 角度(0-180)\n"
            "推荐使用 self.garden.set_servo 代替本工具，用 hang/collect 语义更直观。",
            PropertyList({
                Property("device", kPropertyTypeString),
                Property("index", kPropertyTypeInteger),
                Property("angle", kPropertyTypeInteger)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                auto device = properties["device"].value<std::string>();
                int index = properties["index"].value<int>();
                int angle = properties["angle"].value<int>();

                bool success = App_IOT_Set_Servo_ByIndex(device.c_str(), (uint8_t)index, (uint8_t)angle);

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "device", device.c_str());
                cJSON_AddNumberToObject(json, "servo", index);
                cJSON_AddNumberToObject(json, "angle", angle);
                cJSON_AddStringToObject(json, "message", success ? "舵机已设置" : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.all_off",
            "关闭所有设备（灯、继电器、舵机）。\n"
            "广播命令到所有设备，同时关闭所有负载。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                bool success = App_IOT_Broadcast_All_Off();

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "message", success ? "所有设备已关闭" : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.all_on",
            "打开所有设备（灯、继电器）。\n"
            "广播命令到所有设备，同时打开所有负载。\n"
            "注意：舵机不会被设置，保持当前位置。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                bool success = App_IOT_Broadcast_All_On();

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "message", success ? "所有设备已打开" : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.all_lights_off",
            "关闭所有灯。\n"
            "广播命令到所有设备，只关闭所有灯（不影响继电器和舵机）。\n"
            "用于用户说\"关灯\"、\"关闭所有灯\"时。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                bool success = App_IOT_Broadcast_Lights_Off();

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "message", success ? "所有灯已关闭" : "操作失败");

                return json;
            });

        mcp_server.AddTool("self.iot.all_lights_on",
            "打开所有灯。\n"
            "广播命令到所有设备，只打开所有灯（不影响继电器和舵机）。\n"
            "用于用户说\"开灯\"、\"打开所有灯\"时。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                bool success = App_IOT_Broadcast_Lights_On();

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", success);
                cJSON_AddStringToObject(json, "message", success ? "所有灯已打开" : "操作失败");

                return json;
            });
#endif

        mcp_server.AddTool("self.alarm.add_timer",
            "设置定时器提醒（X分钟后提醒）。Args: minutes(分钟数), message(提醒内容)。",
            PropertyList({
                Property("minutes", kPropertyTypeInteger),
                Property("message", kPropertyTypeString)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                int minutes = properties["minutes"].value<int>();
                auto message = properties["message"].value<std::string>();

                int alarm_id = Alarm_Mcp_Add_Timer(minutes, message.c_str());

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", alarm_id > 0);

                if (alarm_id > 0) {
                    cJSON_AddNumberToObject(json, "alarm_id", alarm_id);
                    cJSON_AddNumberToObject(json, "minutes", minutes);
                    cJSON_AddStringToObject(json, "message", message.c_str());
                } else {
                    cJSON_AddStringToObject(json, "error", "定时器创建失败");
                }

                return json;
            });

        mcp_server.AddTool("self.alarm.add_timer_seconds",
            "设置秒级定时器（X秒后提醒）。Args: seconds(秒数), message(提醒内容)。",
            PropertyList({
                Property("seconds", kPropertyTypeInteger),
                Property("message", kPropertyTypeString)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                int seconds = properties["seconds"].value<int>();
                auto message = properties["message"].value<std::string>();

                int alarm_id = Alarm_Mcp_Add_Timer_Seconds(seconds, message.c_str());

                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", alarm_id > 0);

                if (alarm_id > 0) {
                    cJSON_AddNumberToObject(json, "alarm_id", alarm_id);
                    cJSON_AddNumberToObject(json, "seconds", seconds);
                    cJSON_AddStringToObject(json, "message", message.c_str());
                } else {
                    cJSON_AddStringToObject(json, "error", "秒定时器创建失败");
                }

                return json;
            });

        mcp_server.AddTool("self.alarm.add_alarm",
            "设置闹钟提醒（X点X分）。Args: hour(小时0-23), minute(分钟0-59), message, repeat_daily(可选)。",
            PropertyList({
                Property("hour", kPropertyTypeInteger),
                Property("minute", kPropertyTypeInteger),
                Property("message", kPropertyTypeString),
                Property("repeat_daily", kPropertyTypeBoolean)
            }),
            [](const PropertyList& properties) -> ReturnValue {
                int hour = properties["hour"].value<int>();
                int minute = properties["minute"].value<int>();
                auto message = properties["message"].value<std::string>();
                bool repeat_daily = false;
                if (properties["repeat_daily"].has_default_value() == false) {
                    repeat_daily = properties["repeat_daily"].value<bool>();
                }
                
                int alarm_id = Alarm_Mcp_Add_Alarm(hour, minute, message.c_str(), repeat_daily);
                
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", alarm_id > 0);
                
                if (alarm_id > 0) {
                    cJSON_AddNumberToObject(json, "alarm_id", alarm_id);
                    cJSON_AddNumberToObject(json, "hour", hour);
                    cJSON_AddNumberToObject(json, "minute", minute);
                    cJSON_AddStringToObject(json, "message", message.c_str());
                    cJSON_AddBoolToObject(json, "repeat_daily", repeat_daily);
                } else {
                    cJSON_AddStringToObject(json, "error", "闹钟创建失败");
                }
                
                return json;
            });

        mcp_server.AddTool("self.alarm.get_list",
            "获取所有闹钟和定时器列表。当用户说'查看我的闹钟'时使用。",
            PropertyList(),
            [](const PropertyList& properties) -> ReturnValue {
                char buffer[2048];
                Alarm_Mcp_Get_Status_Json(buffer, sizeof(buffer));
                
                cJSON* json = cJSON_Parse(buffer);
                if (json == nullptr) {
                    json = cJSON_CreateObject();
                    cJSON_AddStringToObject(json, "raw", buffer);
                }
                
                return json;
            });

        Register_Garden_Mcp_Tools();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, 
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
        return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }

    virtual std::string GetBoardType() override {
        return "custom-esp32s3-cam";
    }

    virtual std::string GetBoardJson() override {
        return "{\"type\":\"custom-esp32s3-cam\",\"display\":\"oled\",\"mpu6050\":true,\"buzzer\":true}";
    }

    virtual std::string GetDeviceStatusJson() override {
        return "{}";
    }
};

DECLARE_BOARD(CustomEsp32s3Cam)
