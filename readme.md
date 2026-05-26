# 小智智慧庭院

> 基于 ESP32-S3 小智语音主机、ESP32-S3 庭院从机、MQTT、MCP 和 WS2812B 氛围灯带的智慧庭院系统。

## 项目概览

本项目采用“主机负责智能交互，从机负责实时执行”的架构。小智主机运行语音交互、MCP 工具、MQTT 控制器、TTS 播报和蜂鸣器告警；庭院从机连接灯光、继电器、WS2812B 灯带、舵机、土壤湿度、雨滴、DHT11、火灾传感器和求助按钮。

核心能力：

| 能力 | 当前状态 | 说明 |
| --- | --- | --- |
| 小智语音控制 | 已实现 | 通过 `self.garden.*` MCP 工具控制庭院设备 |
| MQTT 主从通信 | 已实现 | 主机、从机、Web 面板统一走 MQTT 总线 |
| WS2812B 氛围灯 | 已实现 | GPIO12 RMT 驱动，支持 RGB、亮度、呼吸、彩虹、警示闪烁 |
| 一键自动模式 | 已实现 | `self.garden.set_auto_mode(true)` 会同时打开全局自动和所有设备自动接管 |
| 单设备自动接管 | 已实现 | 支持风扇、雾化器、自动浇水、遮雨棚独立开关 |
| 手动控制优先 | 已实现 | 小智手动控制某设备后，该设备自动接管自动关闭 |
| 土壤湿度自动浇水 | 已实现 | 干燥启动水泵，湿润/超时/下雨/火灾停止 |
| DHT11 温湿度调节 | 已实现 | 高温开风扇/雾化，湿度回落后自动关闭 |
| 雨滴遮雨棚联动 | 已实现 | 检测降雨后展开智能遮雨棚，并切到雨天灯效 |
| 火灾本地熔断 | 已实现 | 从机中断触发，低电平切断安防总闸，全开灯并上报告警 |
| 火灾解除恢复 | 已实现 | 恢复安防总闸高电平导通，灯带恢复正常暖白 |
| 求助告警 | 已实现 | 求助按钮触发照明、MQTT emergency 和主机播报 |
| Web 控制面板 | 已实现 | `web/index.html` MQTT.js 直连，`web/control-panel` WebSocket 面板 |

一键自动模式说明：

```text
语音：“开启庭院自动模式”
MCP：self.garden.set_auto_mode(enable=true)
效果：
  auto_mode = true
  auto_fan = true
  auto_atomizer = true
  auto_watering = true
  auto_rain_shelter = true
```

如果用户说“关闭水泵”或“小智把雾化器关掉”，对应设备会进入手动优先状态。例如手动关闭水泵后，`auto_watering=false`，土壤自动浇水不会马上抢回控制权。需要恢复时可以说“恢复自动浇水”或调用 `self.garden.set_device_auto("watering", true)`。

## 系统架构

```text
用户语音 / Web 面板 / 后续 App
        |
        v
小智主机 ESP32-S3
  - 小智语音协议、唤醒、TTS
  - MCP 工具注册与自然语言意图落地
  - MQTT IoT 控制器
  - 传感器事件播报
  - 蜂鸣器告警
        |
        | Wi-Fi + MQTT
        v
MQTT Broker
  mqtt://8.134.167.240:1883
  ws://8.134.167.240:9001
        |
        v
庭院从机 ESP32-S3
  - MQTT 接收、响应、心跳、上线声明
  - GPIO/继电器/舵机/WS2812B 刷新
  - ADC、DHT11、火灾、求助、雨滴检测
  - 本地安全联动
        |
        v
西庭灯 / 庭院灯带 / 长廊灯 / 风扇 / 水泵 / 雾化器 / 安防总闸 / 遮雨棚 / 传感器
```

设计原则：

- 主机不直接接庭院负载，庭院执行器全部接从机。
- 安全事件先在从机本地闭环，不依赖云端和主机。
- MQTT 是主机、从机、Web、后续 App 的统一协议总线。
- MCP 默认保留高层 `self.garden.*` 工具，减少大模型选错工具的概率。

## 代码结构

```text
main/
  application.*                         小智主流程、状态机、TTS/告警入口
  mcp_server.*                          MCP 服务端
  protocols/                            小智通信协议
  boards/custom-esp32s3-cam/
    custom_esp32s3_cam.cc               主机板级初始化、MCP 注册、事件话术
    garden_mcp_tool.*                   智慧庭院高层 MCP 工具
    iot_mcp_tool.*                      通用 IoT MCP 兼容工具
    alarm_mcp_tool.*                    闹钟/定时器 MCP
    config.h                            主机 GPIO 和功能开关
  APP/
    Src/app_garden.c                    智慧庭院业务接口和 JSON 命令入口
    Src/iot_controller.c                主机 MQTT 控制器
    Src/app_buzzer.c                    蜂鸣器
    Src/app_mpu6050.c                   姿态传感器
  BSP/
    Src/bsp_iot_controller.c            远程设备表、响应解析、状态缓存

slave/gardenslave/
  main/iot_slave_main.c                 从机入口
  main/APP/Src/mqtt_receive.c           MQTT 订阅、命令解析、响应发布
  main/APP/Src/mqtt_heartbeat.c         心跳
  main/APP/Src/iot_control_task.c       执行器刷新、火灾/求助中断
  main/APP/Src/sensor_adc_task.c        ADC、DHT11、自动控制、传感器上报
  main/APP/Src/app_asr_uart.c           从机本地语音模块告警串口
  main/APP/Inc/sensor_config.h          传感器阈值和自动模式参数

shared/
  mqtt_iot_protocol.h                   主从共用 Topic、命令码、数据包结构

web/
  index.html                            MQTT.js 直连控制面板
  control-panel/                        Node + WebSocket 控制面板
```

## 硬件连接

### 主机 ESP32-S3

| 模块 | 引脚 | 说明 |
| --- | --- | --- |
| I2S MIC | WS=GPIO1, SCK=GPIO2, DIN=GPIO42 | 语音输入 |
| I2S SPK | DOUT=GPIO39, BCLK=GPIO40, LRCK=GPIO41 | 语音输出 |
| OLED I2C | SDA=GPIO47, SCL=GPIO21 | SSD1306 128x64 |
| MPU6050 软件 I2C | SDA=GPIO3, SCL=GPIO46 | 姿态检测 |
| 蜂鸣器 | GPIO14 | `BUZZER_ACTIVE_LEVEL=0`，低电平有效 |
| 内置 LED | GPIO48 | 状态指示 |
| 调试日志 UART0 | TX=GPIO43, RX=GPIO44 | 主控制台日志 |

主机只接交互硬件，不接庭院强电/负载。

### 庭院从机 ESP32-S3

| 设备 | 逻辑名 | 引脚 | 类型 | 说明 |
| --- | --- | --- | --- | --- |
| 土壤湿度 | `soil_percent` | GPIO1 / ADC1_CH0 | 模拟输入 | 土壤越湿百分比越高 |
| 雨滴检测 | `rain_percent` | GPIO4 / ADC1_CH3 | 模拟输入 | 雨水越多百分比越高 |
| 西庭灯 | `courtyard_west_light` | GPIO11 | 数字输出 | 普通灯光 |
| 庭院灯带 | `strip_light` | GPIO12 | RMT / WS2812B | 30 颗 GRB 灯带 |
| 长廊灯 | `corridor_long_light` | GPIO13 | 数字输出 | 普通灯光 |
| 风扇 | `fan` | GPIO14 | 继电器 CH0 | 高电平导通 |
| 水泵 | `water_pump` | GPIO15 | 继电器 CH1 | 高电平导通 |
| 雾化器 | `atomizer` | GPIO16 | 继电器 CH2 | 高电平导通 |
| 安防总闸 | `safety_switch` | GPIO17 | 继电器 CH3 | 高电平导通，低电平火灾切断 |
| 智能遮雨棚 | `rain_shelter` / `servo` | GPIO18 | LEDC PWM | 0°收回，180°展开 |
| DHT11 | `temperature` / `humidity` | GPIO38 | 单总线 | 温湿度采集 |
| 火灾传感器 | `fire_status` | GPIO2 | 下降沿中断 | 触发火灾熔断 |
| 求助按钮 | `help_status` | GPIO48 | 下降沿中断 | 触发求助告警 |
| ASR 语音模块 | - | GPIO43 | UART1 TX | 从机本地告警语音模块 |

硬件安全注意：

- 安防总闸继电器是高电平导通：上电默认 GPIO17=1，只有火灾确认或明确切断时 GPIO17=0。
- GPIO12 用于 WS2812B。GPIO12 在 ESP32 系列属于敏感启动相关引脚，硬件上必须保证上电时灯带/电平转换电路不会把 GPIO12 拉高。
- WS2812B 建议使用 5V 独立供电、共地、数据线串 330Ω~470Ω、5V 入口并 1000µF 电容。
- ESP32-S3 GPIO 输出 3.3V，长灯带建议用 74AHCT125、74HCT245 或 SN74LV1T34 做电平转换。

## 设备语义

| 中文名称 | 推荐键 | 兼容键 | 说明 |
| --- | --- | --- | --- |
| 西庭灯 | `courtyard_west_light` | `corridor_light` | 普通灯光 |
| 庭院灯带 | `strip_light` | - | WS2812B 氛围灯 |
| 长廊灯 | `corridor_long_light` | `veg_light` | 普通灯光 |
| 风扇 | `fan` | - | 温度/湿度自动联动 |
| 水泵 | `water_pump` | `watering` | 土壤湿度自动浇水 |
| 雾化器 | `atomizer` | `mist` | 高温/干燥自动联动 |
| 安防总闸 | `safety_switch` | - | 高电平正常供电，低电平切断 |
| 智能遮雨棚 | `rain_shelter` | `servo`, `canopy` | 雨滴自动展开 |

## 自动模式

自动模式分两层：

| 状态字段 | 含义 |
| --- | --- |
| `auto_mode` | 全局自动总开关 |
| `auto_fan` | 风扇是否允许被温湿度自动接管 |
| `auto_atomizer` | 雾化器是否允许被温湿度自动接管 |
| `auto_watering` | 水泵是否允许被土壤湿度自动接管 |
| `auto_rain_shelter` | 遮雨棚是否允许被雨滴传感器自动接管 |

一键开启全部自动：

```json
{
  "device": "auto_mode",
  "value": true
}
```

或使用 MCP：

```text
self.garden.set_auto_mode(enable=true)
```

关闭全局自动：

```json
{
  "device": "auto_mode",
  "value": false
}
```

单独恢复某个设备自动接管：

```json
{
  "device": "device_auto",
  "action": "set",
  "value": {
    "target": "watering",
    "enable": true
  }
}
```

自动控制阈值来自 `slave/gardenslave/main/APP/Inc/sensor_config.h`：

| 参数 | 当前值 | 说明 |
| --- | --- | --- |
| `RAIN_THRESHOLD_PERCENT` | 60 | 雨滴百分比超过该值判定为下雨 |
| `RAIN_TRIGGER_COUNT` | 2 | 连续触发次数，过滤瞬时误报 |
| `SOIL_WATER_START_PERCENT` | 45 | 土壤低于该湿度开始浇水 |
| `SOIL_WATER_STOP_PERCENT` | 60 | 土壤达到该湿度停止浇水 |
| `WATER_PUMP_MAX_RUN_MS` | 90000 | 单次自动浇水最长 90 秒 |
| `WATER_PUMP_COOLDOWN_MS` | 30000 | 自动浇水停止后冷却 30 秒 |
| `TEMP_COOLING_ON_C` | 32.0 | 高温自动降温启动 |
| `TEMP_COOLING_OFF_C` | 29.0 | 温度回落后允许关闭降温 |
| `HUMIDITY_DRY_ON_PERCENT` | 35.0 | 空气干燥自动开雾化 |
| `HUMIDITY_DRY_OFF_PERCENT` | 55.0 | 湿度恢复后关闭雾化 |
| `HUMIDITY_FAN_ON_PERCENT` | 80.0 | 高湿度自动开风扇 |
| `HUMIDITY_FAN_OFF_PERCENT` | 70.0 | 湿度回落后关闭风扇 |

优先级规则：

1. 火灾最高优先级：无论自动/手动状态如何，立即切断总闸，关闭水泵和雾化器，全开照明，灯带红色警示。
2. 小智手动控制优先：手动控制风扇、水泵、雾化器、遮雨棚后，对应单设备自动接管关闭。
3. 全局自动模式控制所有自动逻辑是否运行。
4. 单设备自动接管决定具体设备是否被自动逻辑控制。

## 氛围灯场景

WS2812B 灯带支持 RGB、亮度和 4 种效果：

| effect | 值 | 说明 |
| --- | --- | --- |
| `solid` | 0 | 常亮 |
| `breathe` | 1 | 呼吸 |
| `rainbow` | 2 | 彩虹 |
| `alert` | 3 | 警示闪烁 |

内置场景：

| scene | 颜色/效果 | 联动 |
| --- | --- | --- |
| `normal` | 暖白 `255,185,110`，亮度 35，常亮 | 火灾解除后恢复 |
| `night` | 更低亮暖白，亮度 24，常亮 | 关闭风扇和雾化器 |
| `hot` | 浅蓝，亮度 50，常亮 | 打开风扇和雾化器 |
| `relax` | 柔和绿色，亮度 40，呼吸 | 休闲氛围 |
| `party` | 白色基准，亮度 60，彩虹 | 聚会氛围 |
| `rain` | 冷蓝，亮度 32，呼吸 | 雨天氛围 |
| `safe` | 冷白，亮度 70，常亮 | 安全照明，关闭水泵/雾化器，总闸导通 |

灯带色序校准：

```text
self.garden.calibrate_strip()
```

执行后依次显示红、绿、蓝，最后回到 `normal`。如果实际颜色不一致，需要检查 WS2812B 灯带色序或驱动配置。

## 火灾、求助、雨天事件

### 火灾熔断

触发链路：

```text
火灾传感器 GPIO2 下降沿
  -> 从机 ISR 通知 iot_ctrl
  -> fire_status = FIRE_CONFIRMED
  -> GPIO17 = 0，安防总闸切断
  -> 水泵/雾化器关闭
  -> 三路照明打开
  -> WS2812B 红色警示闪烁
  -> 发布 xiaozhi/iot/emergency/{mac}
  -> 主机 TTS 播报 + 蜂鸣器报警
```

解除火灾：

```text
self.garden.fire_reset()
```

效果：

- `fire_status` 恢复正常。
- GPIO17 拉高，安防总闸恢复导通。
- 灯带恢复 `normal` 暖白常亮。
- 不主动打开水泵/雾化器，避免解除火警后误启动负载。

### 求助

求助按钮 GPIO48 下降沿触发：

- 从机全开照明。
- 灯带橙红警示。
- 发布 `emergency_type=1`。
- 主机播报求助提示并蜂鸣。

### 雨天

雨滴百分比连续超过阈值后：

- 如果 `auto_mode=true` 且 `auto_rain_shelter=true`，遮雨棚展开到 180°。
- 灯带切到 `rain` 场景。
- 发布 `emergency_type=2`，主机播报雨天提示。

## MCP 工具

默认启用的庭院高层工具：

| 工具 | 参数 | 用途 |
| --- | --- | --- |
| `self.garden.control` | `target`, `action` | 统一控制灯、风扇、水泵、雾化器、总闸、遮雨棚、一键灯光/设备 |
| `self.garden.set_strip_rgb` | `red`, `green`, `blue`, `brightness`, `effect` | 精确设置灯带颜色 |
| `self.garden.set_ambience_scene` | `scene` | 设置氛围场景 |
| `self.garden.calibrate_strip` | 无 | 一键校准灯带红绿蓝色序 |
| `self.garden.emergency` | 无 | 触发火灾紧急响应 |
| `self.garden.fire_reset` | 无 | 解除火灾警报并恢复正常氛围 |
| `self.garden.get_status` | 无 | 获取设备状态 |
| `self.garden.get_sensors` | 无 | 获取传感器状态 |
| `self.garden.set_auto_mode` | `enable` | 一键开启/关闭全局自动模式；开启时恢复全部单设备自动接管 |
| `self.garden.set_device_auto` | `target`, `enable` | 单独设置某设备自动接管 |
| `self.garden.all` | `scope`, `action` | 一键控制全部灯光或全部用户设备 |

`self.garden.control` 参数：

| target | action |
| --- | --- |
| `courtyard_west_light`, `corridor_light` | `on` / `off` |
| `strip_light` | `on` / `off` |
| `corridor_long_light`, `veg_light` | `on` / `off` |
| `fan` | `on` / `off` |
| `water_pump` | `on` / `off` |
| `atomizer` | `on` / `off` |
| `safety_switch` | `normal` / `cut_off` |
| `rain_shelter`, `servo`, `canopy` | `deploy` / `retract` |
| `all_lights` | `on` / `off` |
| `all_devices` | `on` / `off` |

可选细粒度庭院工具：

```c
#define ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS 1
```

开启后会额外注册：

- `self.garden.set_fan`
- `self.garden.set_servo`
- `self.garden.set_water_pump`
- `self.garden.set_atomizer`
- `self.garden.set_safety_switch`
- `self.garden.all_lights`

兼容/调试工具：

```c
#define ENABLE_LEGACY_IOT_MCP_TOOLS 1
```

开启后会保留通用底层 `self.iot.*` 调试工具。正式演示建议优先使用 `self.garden.*`。

主机还包含：

| 工具前缀 | 用途 |
| --- | --- |
| `self.iot.*` | 通用 IoT 设备调试、发现、状态查询 |
| `self.alarm.*` | 闹钟和定时器 |
| `self.buzzer.*` | 主机蜂鸣器 |
| `self.mpu6050.*` | 姿态传感器数据 |

## MQTT 协议

Broker：

| 项 | 值 |
| --- | --- |
| TCP | `mqtt://8.134.167.240:1883` |
| WebSocket | `ws://8.134.167.240:9001` |
| 主机 Client ID | `xiaozhi_master_esp32` |
| 从机 Client ID | `xiaozhi_slave_<MAC>` |
| Keepalive | 30s |
| 自动重连 | 开启 |

Topic：

| Topic | 方向 | Payload | 说明 |
| --- | --- | --- | --- |
| `xiaozhi/iot/cmd/broadcast` | 主机/Web -> 从机 | `iot_command_packet_t` | 广播命令 |
| `xiaozhi/iot/cmd/10` | 主机/Web -> 庭院从机 | `iot_command_packet_t` | 庭院从机单设备命令 |
| `xiaozhi/iot/resp/{mac}` | 从机 -> 主机/Web | `iot_command_packet_t` | 命令响应 |
| `xiaozhi/iot/heartbeat/{mac}` | 从机 -> 主机/Web | `iot_heartbeat_packet_t` | 心跳 |
| `xiaozhi/iot/announce/{mac}` | 从机 -> 主机/Web | `iot_announce_v2_packet_t` | 上线声明 |
| `xiaozhi/iot/sensor/{mac}` | 从机 -> 主机/Web | JSON 或二进制传感器包 | 传感器状态 |
| `xiaozhi/iot/emergency/{mac}` | 从机 -> 主机/Web | JSON | 火灾、求助、雨天事件 |
| `xiaozhi/garden/cmd` | Web/App -> 主机 | JSON | 高层庭院命令 |
| `xiaozhi/garden/status` | 主机 -> Web/App | JSON | 聚合设备状态 |
| `xiaozhi/garden/sensors` | 主机 -> Web/App | JSON | 聚合传感器状态 |

二进制命令包：

```c
typedef struct {
    uint8_t command;
    uint8_t device_id;
    uint8_t gpio_index;
    uint8_t value;
    uint8_t reserved[4];
} __attribute__((packed)) iot_command_packet_t;
```

命令码：

| 命令 | 值 | 说明 |
| --- | --- | --- |
| `IOT_CMD_SET_GPIO` | `0x01` | 设置通用 GPIO |
| `IOT_CMD_GET_GPIO` | `0x02` | 查询单个 GPIO |
| `IOT_CMD_GET_ALL_GPIO` | `0x03` | 查询全部 GPIO |
| `IOT_CMD_HEARTBEAT` | `0x04` | 心跳 |
| `IOT_CMD_RESPONSE` | `0x05` | 命令响应 |
| `IOT_CMD_DISCOVER` | `0x06` | 发现设备 |
| `IOT_CMD_ANNOUNCE` | `0x07` | 旧上线声明 |
| `IOT_CMD_ANNOUNCE_V2` | `0x08` | 新上线声明，带设备名 |
| `IOT_CMD_SET_SERVO` | `0x10` | 设置舵机角度 |
| `IOT_CMD_SET_LIGHT` | `0x11` | 设置灯光 |
| `IOT_CMD_SET_RELAY` | `0x12` | 设置继电器 |
| `IOT_CMD_SENSOR_REPORT` | `0x13` | 二进制传感器上报 |
| `IOT_CMD_BROADCAST_ALL_OFF` | `0x30` | 全部用户设备关闭，总闸保持导通 |
| `IOT_CMD_BROADCAST_ALL_ON` | `0x31` | 全部灯光和继电器打开 |
| `IOT_CMD_BROADCAST_LIGHTS_OFF` | `0x32` | 全部灯光关闭 |
| `IOT_CMD_BROADCAST_LIGHTS_ON` | `0x33` | 全部灯光打开 |
| `IOT_CMD_EMERGENCY` | `0x34` | 火灾紧急联动 |
| `IOT_CMD_SET_AUTO_MODE` | `0x35` | 全局自动模式；开启时恢复全部单设备自动接管 |
| `IOT_CMD_FIRE_RESET` | `0x36` | 火灾解除，恢复总闸和正常灯效 |
| `IOT_CMD_RAIN_ALERT` | `0x37` | 雨天事件 |
| `IOT_CMD_HELP_ALERT` | `0x38` | 求助事件 |
| `IOT_CMD_SET_STRIP_RGB` | `0x39` | 设置 WS2812B RGB/亮度/效果 |
| `IOT_CMD_SET_AMBIENCE_SCENE` | `0x3A` | 设置氛围场景 |
| `IOT_CMD_SET_DEVICE_AUTO` | `0x3B` | 设置单设备自动接管 |

庭院从机全局索引：

| gpio_index | 设备 |
| --- | --- |
| 0 | 西庭灯 |
| 1 | 庭院 WS2812B 灯带 |
| 2 | 长廊灯 |
| 3 | 风扇继电器 |
| 4 | 水泵继电器 |
| 5 | 雾化器继电器 |
| 6 | 安防总闸继电器 |
| 7 | 智能遮雨棚舵机 |

继电器局部索引：

| relay_index | 设备 |
| --- | --- |
| 0 | 风扇 |
| 1 | 水泵 |
| 2 | 雾化器 |
| 3 | 安防总闸 |

单设备自动接管 ID：

| auto_id | target | 说明 |
| --- | --- | --- |
| 0 | `fan` | 风扇温湿度自动接管 |
| 1 | `atomizer` | 雾化器温湿度自动接管 |
| 2 | `watering` | 水泵土壤湿度自动接管 |
| 3 | `rain_shelter` | 遮雨棚雨滴自动接管 |

灯带 RGB 命令格式：

```text
command     = IOT_CMD_SET_STRIP_RGB
gpio_index  = 1
value       = brightness 0-100
reserved[0] = red
reserved[1] = green
reserved[2] = blue
reserved[3] = effect
```

## JSON 高层命令

Topic：`xiaozhi/garden/cmd`

基本格式：

```json
{
  "device": "fan",
  "action": "on",
  "value": true
}
```

常用命令：

```json
{"device":"courtyard_west_light","action":"on"}
{"device":"strip_light","action":"off"}
{"device":"corridor_long_light","action":"on"}
{"device":"fan","action":"on"}
{"device":"water_pump","action":"off"}
{"device":"atomizer","action":"on"}
{"device":"safety_switch","action":"normal"}
{"device":"safety_switch","action":"cut_off"}
{"device":"rain_shelter","action":"deploy"}
{"device":"rain_shelter","action":"retract"}
{"device":"all_lights","action":"on"}
{"device":"all_lights","action":"off"}
{"device":"all_devices","action":"on"}
{"device":"all_devices","action":"off"}
{"device":"auto_mode","value":true}
{"device":"auto_mode","value":false}
{"device":"emergency","action":"fire"}
{"device":"emergency","action":"fire_reset"}
```

设置单设备自动：

```json
{
  "device": "device_auto",
  "action": "set",
  "value": {
    "target": "watering",
    "enable": true
  }
}
```

设置灯带 RGB：

```json
{
  "device": "strip_light",
  "action": "rgb",
  "value": {
    "red": 120,
    "green": 200,
    "blue": 255,
    "brightness": 50,
    "effect": 0
  }
}
```

设置氛围场景：

```json
{"device":"ambience","action":"scene","value":"normal"}
{"device":"ambience","action":"scene","value":"night"}
{"device":"ambience","action":"scene","value":"hot"}
{"device":"ambience","action":"scene","value":"relax"}
{"device":"ambience","action":"scene","value":"party"}
{"device":"ambience","action":"scene","value":"rain"}
{"device":"ambience","action":"scene","value":"safe"}
```

## 状态 JSON

Topic：`xiaozhi/garden/status`

示例：

```json
{
  "online": true,
  "expected": {
    "corridor_light": 0,
    "strip_light": 1,
    "veg_light": 0,
    "strip_rgb": [255, 185, 110],
    "strip_brightness": 35,
    "strip_effect": 0,
    "ambience_scene": "normal",
    "fan": 0,
    "servo": 1,
    "auto_mode": true,
    "auto_fan": true,
    "auto_atomizer": true,
    "auto_watering": true,
    "auto_rain_shelter": true,
    "water_pump": 0,
    "atomizer": 0,
    "safety_switch": 1,
    "fire_status": 0
  },
  "actual": {
    "...": "字段同 expected"
  }
}
```

字段说明：

| 字段 | 含义 |
| --- | --- |
| `expected` | 主机最后一次期望状态 |
| `actual` | 从机响应或传感器事件反馈后的实际状态 |
| `safety_switch=1` | 总闸导通，庭院负载正常供电 |
| `safety_switch=0` | 总闸切断，通常为火灾熔断 |
| `servo=1` | 遮雨棚展开 |
| `servo=0` | 遮雨棚收回 |

Topic：`xiaozhi/garden/sensors`

示例：

```json
{
  "soil": 48,
  "rain_percent": 12,
  "rain_adc_mv": 2600,
  "rain_state": "dry",
  "fire_status": 0,
  "help_status": 0,
  "temperature": 26.5,
  "humidity": 58.0
}
```

从机原始传感器上报 Topic：`xiaozhi/iot/sensor/{mac}`

```json
{
  "soil_mv": 1450,
  "soil_percent": 48,
  "rain_mv": 2600,
  "rain_percent": 12,
  "fire_status": 0,
  "rain_status": 0,
  "help_status": 0,
  "temperature": 26.5,
  "humidity": 58.0
}
```

紧急事件 Topic：`xiaozhi/iot/emergency/{mac}`

```json
{
  "type": "emergency",
  "device_id": "AABBCCDDEEFF",
  "emergency_type": 0,
  "detail": "Fire detected by smoke sensor"
}
```

| emergency_type | 含义 | 主机行为 |
| --- | --- | --- |
| 0 | 火灾 | TTS 播报、蜂鸣器急促报警、更新火灾状态 |
| 1 | 求助 | TTS 播报、蜂鸣器提示、照明提示 |
| 2 | 雨天 | TTS 播报、提示遮雨棚已展开 |

## Web 面板

### MQTT.js 直连面板

文件：

```text
web/index.html
```

使用方式：

1. 浏览器打开 HTML。
2. MQTT WebSocket 默认地址 `ws://8.134.167.240:9001`。
3. 点击连接后，可控制灯光、氛围灯、设备、自动接管，并查看传感器。

### Node + WebSocket 面板

目录：

```text
web/control-panel/
```

用途：

- 适合后续扩展登录、设备列表、历史记录和更复杂 UI。
- 当前已同步遮雨棚语义。

## 构建与烧录

主机工程：

```powershell
$env:IDF_PATH='E:\MCU\esp32\.espressif\v5.5.3\esp-idf'
& 'C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe' `
  'E:\MCU\esp32\.espressif\v5.5.3\esp-idf\tools\idf.py' build
```

主机烧录：

```powershell
& 'C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe' `
  'E:\MCU\esp32\.espressif\v5.5.3\esp-idf\tools\idf.py' -p COM16 flash monitor
```

庭院从机：

```powershell
cd slave/gardenslave
& 'C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe' `
  'E:\MCU\esp32\.espressif\v5.5.3\esp-idf\tools\idf.py' build
```

如果从机已有 `build` 目录使用了不同 CMake 生成器，可以新建临时构建目录验证：

```powershell
& 'C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe' `
  'E:\MCU\esp32\.espressif\v5.5.3\esp-idf\tools\idf.py' -B build-codex build
```

串口日志：

```powershell
& 'C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe' `
  'E:\MCU\esp32\.espressif\v5.5.3\esp-idf\tools\idf.py' -p COM16 monitor
```

如果看不到串口日志：

- 确认没有其他串口工具占用 COM 口。
- 关闭 VS Code 串口监视器、旧的 `idf_monitor.py`、串口助手。
- 确认当前固件配置的控制台输出通道和你连接的串口一致。
- `ClearCommError failed PermissionError(13)` 通常就是串口被占用。

## 现场演示脚本

建议演示顺序：

1. 说“打开庭院自动模式”：观察 `auto_mode` 和四个单设备自动接管全部为 true。
2. 说“夜间模式”：灯带变暖白低亮，风扇/雾化器关闭。
3. 说“我有点闷热”：灯带变浅蓝，风扇和雾化器打开。
4. 说“校准灯带颜色”：灯带依次显示红、绿、蓝，再恢复暖白。
5. 模拟土壤干燥：水泵自动启动，到湿度阈值或超时停止。
6. 模拟下雨：遮雨棚展开，灯带切到雨天冷蓝呼吸。
7. 手动关闭水泵：展示 `auto_watering=false`，自动模式不会抢回控制。
8. 说“恢复自动浇水”：展示 `auto_watering=true`。
9. 触发火灾：总闸 GPIO17 拉低，水泵/雾化器关闭，全灯打开，灯带红色警示，主机播报。
10. 说“解除火灾警报”：总闸 GPIO17 恢复高电平，灯带恢复正常暖白。

## 验证记录

最近一次代码验证：

| 工程 | 命令 | 结果 |
| --- | --- | --- |
| 主机 | `idf.py build` | 通过，生成 `build/xiaozhi.bin` |
| 从机 | `idf.py -B build-codex build` | 通过，生成 `iot_slave_garden.bin`，验证后已删除临时目录 |
| Git 检查 | `git diff --check` | 通过，仅有 Windows CRLF 提示 |

## 国奖展示亮点

项目现在的亮点不只是“能开关设备”，而是形成了完整闭环：

- 自然语言到 MCP，再到 MQTT，再到从机执行器的完整链路。
- 火灾安全本地闭环，不依赖网络。
- 安防总闸高电平导通、低电平熔断的硬件语义清晰。
- WS2812B 氛围灯把“夜间、闷热、雨天、安全、聚会”变成可感知场景。
- 自动模式支持全局一键开启，也支持单设备接管和手动优先。
- 土壤湿度、水泵、DHT11、雾化器、风扇、雨滴、遮雨棚形成多传感器联动。
- MQTT Topic 和 JSON 接口已能支撑后续 App 或云端平台接入。

后续最值得加分的优化：

1. 给 Web/App 增加历史曲线：土壤湿度、温度、湿度、雨滴百分比。
2. 加设备健康诊断：从机离线、传感器异常、继电器动作超时。
3. 增加本地 NVS 配置阈值：让评委现场调节浇水/温湿度阈值。
4. 增加外壳和接线标识：强弱电隔离、保险丝、继电器端子标注。
5. 增加演示展板：系统架构图、协议链路图、安全联动时序图。
