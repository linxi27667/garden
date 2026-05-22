#include "garden_mcp_tool.h"
#include "app_garden.h"
#include "iot_controller.h"
#include "mqtt_iot_protocol.h"
#include "mcp_server.h"
#include "esp_log.h"

#include <string>

static const char* TAG = "GARDEN_MCP";

#ifndef ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS
#define ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS 0
#endif

static cJSON* Garden_Control_Result(bool success, const std::string& target,
                                    const std::string& action, const char* message) {
    cJSON* json = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "success", success);
    cJSON_AddStringToObject(json, "target", target.c_str());
    cJSON_AddStringToObject(json, "action", action.c_str());
    cJSON_AddStringToObject(json, "message", message);
    return json;
}

static bool Is_On_Action(const std::string& action) {
    return action == "on" || action == "open" || action == "normal" || action == "connect";
}

static bool Is_Off_Action(const std::string& action) {
    return action == "off" || action == "close" || action == "cut_off" || action == "disconnect";
}

void Register_Garden_Mcp_Tools(void) {
    auto& mcp_server = McpServer::GetInstance();

    mcp_server.AddTool("self.garden.control",
        "智慧庭院统一控制工具，用一个入口控制常用执行器，避免选择过多底层 MCP。\n"
        "Args:\n"
        "  target: courtyard_west_light/strip_light/corridor_long_light/fan/water_pump/atomizer/safety_switch/servo/all_lights/all_devices\n"
        "  action: on/off，safety_switch 用 normal/cut_off，servo 用 hang/collect\n"
        "安全语义：safety_switch normal=GPIO17高电平导通，cut_off=GPIO17低电平切断。",
        PropertyList({
            Property("target", kPropertyTypeString),
            Property("action", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto target = properties["target"].value<std::string>();
            auto action = properties["action"].value<std::string>();

            if (target == "corridor_light" || target == "courtyard_west_light") {
                if (Is_On_Action(action)) App_Garden_Set_Corridor_Light(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Corridor_Light(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "西庭灯只支持 on/off");
            } else if (target == "strip_light") {
                if (Is_On_Action(action)) App_Garden_Set_Strip_Light(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Strip_Light(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "灯带开关只支持 on/off，颜色请用 self.garden.set_strip_rgb");
            } else if (target == "veg_light" || target == "corridor_long_light") {
                if (Is_On_Action(action)) App_Garden_Set_Veg_Light(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Veg_Light(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "长廊灯只支持 on/off");
            } else if (target == "fan") {
                if (Is_On_Action(action)) App_Garden_Set_Fan(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Fan(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "风扇只支持 on/off");
            } else if (target == "water_pump") {
                if (Is_On_Action(action)) App_Garden_Set_Water_Pump(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Water_Pump(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "水泵只支持 on/off");
            } else if (target == "atomizer") {
                if (Is_On_Action(action)) App_Garden_Set_Atomizer(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Atomizer(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "雾化器只支持 on/off");
            } else if (target == "safety_switch") {
                if (Is_On_Action(action)) App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);
                else if (Is_Off_Action(action)) App_Garden_Set_Safety_Switch(GARDEN_SWITCH_OFF);
                else return Garden_Control_Result(false, target, action, "安防总闸只支持 normal/cut_off");
            } else if (target == "servo") {
                if (action == "hang") App_Garden_Set_Servo(GARDEN_SERVO_HANG);
                else if (action == "collect") App_Garden_Set_Servo(GARDEN_SERVO_COLLECT);
                else return Garden_Control_Result(false, target, action, "晾衣架只支持 hang/collect");
            } else if (target == "all_lights") {
                if (Is_On_Action(action)) App_Garden_All_Lights_On();
                else if (Is_Off_Action(action)) App_Garden_All_Lights_Off();
                else return Garden_Control_Result(false, target, action, "全部灯光只支持 on/off");
            } else if (target == "all_devices") {
                if (Is_On_Action(action)) App_Garden_All_Devices_On();
                else if (Is_Off_Action(action)) App_Garden_All_Devices_Off();
                else return Garden_Control_Result(false, target, action, "全部设备只支持 on/off");
            } else {
                return Garden_Control_Result(false, target, action, "未知设备，请使用智慧庭院支持的 target");
            }

            return Garden_Control_Result(true, target, action, "控制命令已下发");
        });

    mcp_server.AddTool("self.garden.set_strip_rgb",
        "精确设置智慧庭院 WS2812B 氛围灯带颜色。\n"
        "Args:\n"
        "  red: 红色通道 0-255\n"
        "  green: 绿色通道 0-255\n"
        "  blue: 蓝色通道 0-255\n"
        "  brightness: 亮度 0-100，建议不超过70\n"
        "  effect: 效果 solid/breathe/rainbow/alert\n"
        "适合用户明确要求某种颜色、亮度或动态效果时使用。",
        PropertyList({
            Property("red", kPropertyTypeInteger, 0, 255),
            Property("green", kPropertyTypeInteger, 0, 255),
            Property("blue", kPropertyTypeInteger, 0, 255),
            Property("brightness", kPropertyTypeInteger, 0, 100),
            Property("effect", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            int red = properties["red"].value<int>();
            int green = properties["green"].value<int>();
            int blue = properties["blue"].value<int>();
            int brightness = properties["brightness"].value<int>();
            auto effect_name = properties["effect"].value<std::string>();

            uint8_t effect = IOT_STRIP_EFFECT_SOLID;
            if (effect_name == "breathe" || effect_name == "breath") {
                effect = IOT_STRIP_EFFECT_BREATHE;
            } else if (effect_name == "rainbow") {
                effect = IOT_STRIP_EFFECT_RAINBOW;
            } else if (effect_name == "alert" || effect_name == "blink") {
                effect = IOT_STRIP_EFFECT_ALERT;
            } else if (effect_name != "solid") {
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", false);
                cJSON_AddStringToObject(json, "message", "未知效果，可选: solid, breathe, rainbow, alert");
                return json;
            }

            bool ok = App_Garden_Set_Strip_RGB((uint8_t)red, (uint8_t)green, (uint8_t)blue,
                                               (uint8_t)brightness, effect);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", ok);
            cJSON_AddNumberToObject(json, "red", red);
            cJSON_AddNumberToObject(json, "green", green);
            cJSON_AddNumberToObject(json, "blue", blue);
            cJSON_AddNumberToObject(json, "brightness", brightness);
            cJSON_AddStringToObject(json, "effect", effect_name.c_str());
            cJSON_AddStringToObject(json, "message", ok ? "氛围灯颜色已设置" : "氛围灯颜色设置失败");
            return json;
        });

    mcp_server.AddTool("self.garden.set_ambience_scene",
        "设置智慧庭院氛围场景，并按场景联动风扇、雾化器、水泵等设备。\n"
        "Args:\n"
        "  scene: night=夜间暖白, hot=闷热降温浅蓝并打开风扇/雾化器, relax=休闲呼吸绿, party=彩虹, rain=雨天冷蓝, safe=安全照明\n"
        "用户说夜间模式、有点闷热、放松一下、聚会氛围、雨天氛围、安全照明时优先使用。",
        PropertyList({
            Property("scene", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto scene = properties["scene"].value<std::string>();
            bool ok = App_Garden_Set_Ambience_Scene(scene.c_str());

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", ok);
            cJSON_AddStringToObject(json, "scene", scene.c_str());
            cJSON_AddStringToObject(json, "message", ok ? "氛围场景已应用" : "未知氛围场景，可选: night, hot, relax, party, rain, safe");
            return json;
        });

#if ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS
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

    mcp_server.AddTool("self.garden.set_safety_switch",
        "控制智慧庭院安防总闸继电器。\n"
        "硬件语义：GPIO17 高电平=继电器导通=庭院总闸正常供电；GPIO17 低电平=继电器断开=切断庭院负载。\n"
        "只有火灾熔断或明确要求切断总闸时才设置 cut_off；恢复正常、火灾解除、上电默认应设置 normal。\n"
        "Args:\n"
        "  action: normal=总闸导通/正常供电, cut_off=切断总闸/火灾熔断",
        PropertyList({
            Property("action", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto action = properties["action"].value<std::string>();
            bool normal = false;

            if (action == "normal" || action == "on" || action == "connect" || action == "power_on") {
                normal = true;
            } else if (action == "cut_off" || action == "off" || action == "trip" || action == "disconnect") {
                normal = false;
            } else {
                cJSON* json = cJSON_CreateObject();
                cJSON_AddBoolToObject(json, "success", false);
                cJSON_AddStringToObject(json, "message", "无效操作，请用 normal 或 cut_off");
                return json;
            }

            App_Garden_Set_Safety_Switch(normal ? GARDEN_SWITCH_ON : GARDEN_SWITCH_OFF);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "action", normal ? "normal" : "cut_off");
            cJSON_AddNumberToObject(json, "gpio_level", normal ? 1 : 0);
            cJSON_AddStringToObject(json, "message", normal ? "安防总闸已导通，庭院负载正常供电" : "安防总闸已切断，庭院负载断电");
            return json;
        });
#endif

    mcp_server.AddTool("self.garden.emergency",
        "触发智慧庭院火灾紧急响应。\n"
        "联动动作：切断安防总闸、停止水泵和雾化器、全开照明灯、MQTT紧急广播。\n"
        "安防总闸硬件语义：高电平=导通正常供电，低电平=火灾熔断切断负载。",
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
            App_Garden_Set_Safety_Switch(GARDEN_SWITCH_ON);

            cJSON* json = cJSON_CreateObject();
            cJSON_AddBoolToObject(json, "success", true);
            cJSON_AddStringToObject(json, "message", "火灾警报已解除，安防总闸已恢复高电平导通，设备恢复正常运行");
            return json;
        });

    mcp_server.AddTool("self.garden.get_status",
        "获取智慧庭院所有设备的状态。\n"
        "返回西庭灯、庭院灯带、长廊灯、风扇、舵机、水泵、雾化器、安防总闸、火灾状态的实时数据。",
        PropertyList(),
        [](const PropertyList& properties) -> ReturnValue {
            char buffer[1536];
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

    mcp_server.AddTool("self.garden.all",
        "一键打开或关闭智慧庭院全部灯光或全部用户设备。\n"
        "Args:\n"
        "  scope: lights=只控制全部灯光, devices=控制灯光/风扇/水泵/雾化器/晾衣架等全部用户设备\n"
        "  action: on=开启, off=关闭\n"
        "注意：关闭全部设备不会切断安防总闸，安防总闸保持高电平导通；火灾熔断请使用 self.garden.emergency。",
        PropertyList({
            Property("scope", kPropertyTypeString),
            Property("action", kPropertyTypeString)
        }),
        [](const PropertyList& properties) -> ReturnValue {
            auto scope = properties["scope"].value<std::string>();
            auto action = properties["action"].value<std::string>();

            if (!Is_On_Action(action) && !Is_Off_Action(action)) {
                return Garden_Control_Result(false, scope, action, "action 只支持 on/off");
            }

            if (scope == "lights" || scope == "all_lights") {
                if (Is_On_Action(action)) App_Garden_All_Lights_On();
                else App_Garden_All_Lights_Off();
            } else if (scope == "devices" || scope == "all_devices") {
                if (Is_On_Action(action)) App_Garden_All_Devices_On();
                else App_Garden_All_Devices_Off();
            } else {
                return Garden_Control_Result(false, scope, action, "scope 只支持 lights/devices");
            }

            return Garden_Control_Result(true, scope, action, "一键控制命令已下发");
        });

#if ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS
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
#endif

    ESP_LOGI(TAG, "Garden MCP tools registered (optimized mode, %d tools)", 9);
}
