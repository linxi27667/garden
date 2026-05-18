#include "garden_mcp_tool.h"
#include "app_garden.h"
#include "iot_controller.h"
#include "mcp_server.h"
#include "esp_log.h"

static const char* TAG = "GARDEN_MCP";

void Register_Garden_Mcp_Tools(void) {
    auto& mcp_server = McpServer::GetInstance();

    mcp_server.AddTool("self.garden.set_light",
        "控制智慧庭院的灯光设备。\n"
        "Args:\n"
        "  light: 灯光名称(courtyard_west_light/corridor_light=西庭灯, strip_light=庭院灯带, corridor_long_light/veg_light=长廊灯)\n"
        "  value: true=开/false=关\n"
        "通过 MQTT 控制庭院从机设备。",
        PropertyList({
            Property("light", kPropertyTypeString),
            Property("value", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto light = properties["light"].value<std::string>();
            bool value = properties["value"].value<bool>();

            if (light == "corridor_light" || light == "courtyard_west_light") {
                App_Garden_Set_Corridor_Light(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);
            } else if (light == "strip_light") {
                App_Garden_Set_Strip_Light(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);
            } else if (light == "veg_light" || light == "corridor_long_light") {
                App_Garden_Set_Veg_Light(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);
            } else {
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", false);
                cJSON_AddStringToObject(json, "message", "未知灯光，可选: courtyard_west_light, strip_light, corridor_long_light");
                return json;
            }

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "light", light.c_str());
            cJSON_AddBoolToObject(json, "value", value);
            cJSON_AddStringToObject(json, "message", value ? "灯已打开" : "灯已关闭");
            return json;
        });

    mcp_server.AddTool("self.garden.set_fan",
        "控制智慧庭院的风扇开关。\n"
        "Args:\n"
        "  value: true=开/false=关",
        PropertyList({
            Property("value", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            bool value = properties["value"].value<bool>();
            App_Garden_Set_Fan(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddBoolToObject(json, "value", value);
            cJSON_AddStringToObject(json, "message", value ? "风扇已打开" : "风扇已关闭");
            return json;
        });

    mcp_server.AddTool("self.garden.set_servo",
        "控制智慧庭院的晾衣架舵机。\n"
        "Args:\n"
        "  action: hang=晾衣服(伸出), collect=收衣服(收回)",
        PropertyList({
            Property("action", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto action = properties["action"].value<std::string>();
            std::string desc;

            if (action == "hang") {
                App_Garden_Set_Servo(GARDEN_SERVO_HANG);
                desc = "晾衣服";
            } else if (action == "collect") {
                App_Garden_Set_Servo(GARDEN_SERVO_COLLECT);
                desc = "收衣服";
            } else {
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", false);
                cJSON_AddStringToObject(json, "message", "无效操作，请用hang或collect");
                return json;
            }

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "action", desc.c_str());
            cJSON_AddStringToObject(json, "message",
                action == "hang" ? "衣服已晾好" : "衣服已收进来");
            return json;
        });

    mcp_server.AddTool("self.garden.set_water_pump",
        "控制智慧庭院的循环水泵。\n"
        "Args:\n"
        "  value: true=开/false=关\n"
        "通过继电器CH1控制，火灾时自动关闭。",
        PropertyList({
            Property("value", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            bool value = properties["value"].value<bool>();
            App_Garden_Set_Water_Pump(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddBoolToObject(json, "value", value);
            cJSON_AddStringToObject(json, "message", value ? "水泵已打开" : "水泵已关闭");
            return json;
        });

    mcp_server.AddTool("self.garden.set_atomizer",
        "控制智慧庭院的超声波雾化器。\n"
        "Args:\n"
        "  value: true=开/false=关\n"
        "通过继电器CH2控制，火灾时自动关闭。高温/干燥时自动开启。",
        PropertyList({
            Property("value", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            bool value = properties["value"].value<bool>();
            App_Garden_Set_Atomizer(value ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddBoolToObject(json, "value", value);
            cJSON_AddStringToObject(json, "message", value ? "雾化器已打开" : "雾化器已关闭");
            return json;
        });

    mcp_server.AddTool("self.garden.emergency",
        "触发智慧庭院火灾紧急响应。\n"
        "联动动作：切断安防总闸、停止水泵和雾化器、全开照明灯、MQTT紧急广播。\n"
        "安防总闸走NC常闭端，高电平吸合时切断庭院总电源。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            App_Garden_Fire_Emergency_Response();

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "message", "火灾紧急响应已触发：总闸已切断、水泵雾化器已停止、照明全开、紧急广播已发送");
            return json;
        });

    mcp_server.AddTool("self.garden.fire_reset",
        "解除火灾警报，恢复庭院正常状态。\n"
        "当确认火灾已解除后，使用此工具重置火灾状态，恢复安防总闸和设备正常运行。\n"
        "用户说'火灾解除了'、'恢复正常'、'取消警报'时使用。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            App_IOT_Fire_Reset();
            App_Garden_Set_Safety_Switch(GARDEN_SWITCH_OFF);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "message", "火灾警报已解除，设备恢复正常运行");
            return json;
        });

    mcp_server.AddTool("self.garden.get_status",
        "获取智慧庭院所有设备的状态。\n"
        "返回西庭灯、庭院灯带、长廊灯、风扇、舵机、水泵、雾化器、安防总闸、火灾状态的实时数据。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            char buffer[1024];
            App_Garden_Get_Status_Json(buffer, sizeof(buffer));

            cJSON* json = cJSON_Parse(buffer);
            if (json == nullptr) {
                json = cJSON_CreateObject();
                cJSON_AddStringToObject(json, "raw", buffer);
            }
            return json;
        });

    mcp_server.AddTool("self.garden.get_sensors",
        "获取智慧庭院传感器数据。\n"
        "返回土壤湿度、雨滴状态、火灾状态、求助状态、温度、湿度。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            char buffer[768];
            App_Garden_Get_Sensors_Json(buffer, sizeof(buffer));

            cJSON* json = cJSON_Parse(buffer);
            if (json == nullptr) {
                json = cJSON_CreateObject();
                cJSON_AddStringToObject(json, "raw", buffer);
            }
            return json;
        });

    mcp_server.AddTool("self.garden.set_auto_mode",
        "设置智慧庭院自动模式。\n"
        "自动模式下：雨天自动收衣、高温自动开风扇/雾化、干燥自动开雾化。\n"
        "Args:\n"
        "  enable: true=开启自动模式/false=关闭",
        PropertyList({
            Property("enable", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            bool enable = properties["enable"].value<bool>();
            App_Garden_Set_Auto_Mode(enable);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddBoolToObject(json, "auto_mode", enable);
            cJSON_AddStringToObject(json, "message", enable ? "自动模式已开启" : "自动模式已关闭");
            return json;
        });

    mcp_server.AddTool("self.garden.all_lights",
        "控制智慧庭院所有灯的开关。\n"
        "Args:\n"
        "  value: true=全开/false=全关",
        PropertyList({
            Property("value", kPropertyTypeBoolean)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            bool value = properties["value"].value<bool>();
            if (value) {
                App_Garden_All_Lights_On();
            } else {
                App_Garden_All_Lights_Off();
            }

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddBoolToObject(json, "value", value);
            cJSON_AddStringToObject(json, "message", value ? "所有灯已打开" : "所有灯已关闭");
            return json;
        });

    ESP_LOGI(TAG, "Garden MCP tools registered (local + MQTT mode, %d tools)", 11);
}
