# 小智智慧庭院

> 基于 ESP32-S3 小智语音主机、ESP32-S3 庭院从机、MQTT、MCP 和 WS2812B 氛围灯带的智慧庭院系统。

## 项目概览

本项目采用"主机负责智能交互，从机负责实时执行"的架构。小智主机运行语音交互、MCP 工具、MQTT 控制器、TTS 播报和蜂鸣器告警；庭院从机连接灯光、继电器、WS2812B 灯带、舵机、土壤湿度、雨滴、DHT11、火灾传感器和求助按钮。

核心能力：

| 能力 | 状态 | 说明 |
| --- | --- | --- |
| 小智语音控制 | 已实现 | 通过 `self.garden.*` MCP 工具控制庭院设备 |
| MQTT 主从通信 | 已实现 | 主机、从机、Web 面板统一走 MQTT 总线 |
| 成语意境场景 | 已实现 | 7 种四字成语命名的庭院意境，语音切换 |
| WS2812B 氛围灯 | 已实现 | GPIO12 RMT 驱动，RGB、亮度、呼吸、彩虹、警示闪烁 |
| 一键自动模式 | 已实现 | 上电默认关闭，开启后全场景自动接管 |
| 单设备自动接管 | 已实现 | 支持通风扇、雾化器、自动浇水、遮雨棚独立开关 |
| 手动控制优先 | 已实现 | 小智手动控制某设备后，该设备自动接管自动关闭 |
| 土壤湿度自动浇水 | 已实现 | 干燥启动水泵，湿润停止（无超时限制） |
| DHT11 温湿度调节 | 已实现 | 高温开通风扇/雾化，温湿度回落后自动关闭 |
| 雨滴遮雨棚联动 | 已实现 | 降雨展开遮雨棚(90°)+切雨天灯效，雨停自动收回+恢复正常 |
| 火灾本地熔断 | 已实现 | 从机 ADC 电压阈值轮询，连续3次超阈值(1.5s)即触发，抗EMI干扰 |
| 火灾解除恢复 | 已实现 | 火源移除后电压回稳，连续15s低于阈值自动恢复风和日丽 |
| 求助告警 | 已实现 | 求助按钮触发照明、MQTT emergency 和主机播报 |
| Web 控制面板 | 已实现 | `web/index.html` MQTT.js 直连，`web/control-panel` WebSocket 面板 |
| AI 对话实时转发 | 已实现 | 主机将 ASR/TTS 文本通过 `xiaozhi/garden/chat` 实时推送到 App |

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
西庭灯(小黄LED) / 庭院灯带(WS2812B) / 长廊灯(暖黄灯带)
通风扇 / 水泵 / 雾化器 / 安防总闸 / 遮雨棚 / 传感器
```

设计原则：

- 主机不直接接庭院负载，庭院执行器全部接从机。
- 安全事件先在从机本地闭环，不依赖云端和主机。
- 场景管氛围（灯光），自动模式管设备（继电器），两者正交不干扰。
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
| 土壤湿度 | `soil_percent` | GPIO1 / ADC1_CH0 | 模拟输入 | 干燥→低百分比，湿润→高百分比 |
| 雨滴检测 | `rain_percent` | GPIO4 / ADC1_CH3 | 模拟输入 | 雨水越多百分比越高 |
| 西庭灯 | `courtyard_west_light` | GPIO11 | 数字输出 | 小黄色LED，模拟庭院小屋灯火 |
| 庭院灯带 | `strip_light` | GPIO12 | RMT / WS2812B | 30 颗 RGB 灯带，主氛围营造 |
| 长廊灯 | `corridor_long_light` | GPIO13 | 数字输出 | 暖黄色 LED 灯带 |
| 通风扇 | `fan` | GPIO14 | 继电器 CH0 | 高电平导通 |
| 水泵 | `water_pump` | GPIO15 | 继电器 CH1 | 高电平导通 |
| 雾化器 | `atomizer` | GPIO16 | 继电器 CH2 | 高电平导通 |
| 安防总闸 | `safety_switch` | GPIO17 | 继电器 CH3 | 高电平导通，低电平火灾切断 |
| 智能遮雨棚 | `rain_shelter` / `servo` | GPIO18 | LEDC PWM | 0°收回，90°展开 |
| DHT11 | `temperature` / `humidity` | GPIO38 | 单总线 | 温湿度采集 |
| 火灾传感器 | `fire_status` | GPIO2 / ADC1_CH1 | 模拟输入 | AO 引脚，电压越低火越近，阈值轮询检测 |
| 求助按钮 | `help_status` | GPIO48 | 下降沿中断 | 触发求助告警 |
| ASR 语音模块 | - | GPIO21 | UART1 TX | 从机本地告警语音模块 |

硬件安全注意：

- 安防总闸继电器是高电平导通：上电默认 GPIO17=1，只有火灾确认或明确切断时 GPIO17=0。
- GPIO12 用于 WS2812B。GPIO12 在 ESP32 系列属于敏感启动相关引脚，硬件上必须保证上电时灯带/电平转换电路不会把 GPIO12 拉高。
- WS2812B 建议使用 5V 独立供电、共地、数据线串 330Ω~470Ω、5V 入口并 1000µF 电容。
- ESP32-S3 GPIO 输出 3.3V，长灯带建议用 74AHCT125、74HCT245 或 SN74LV1T34 做电平转换。

## 成语意境场景

v3.0 采用中国四字成语命名场景，每个成语都描绘了一幅经典庭院意境。

**设计哲学**：场景只管灯光氛围，自动模式独立管理设备（通风扇/水泵/雾化器/遮雨棚），两者正交不干扰。唯一例外：安全警戒场景会强制关断危险设备。

语音触发：说成语名称即可切换（如"小智，月白风清"）。小智会播报"已切换到月白风清模式"。

| 成语 | 英文键 | 意境 | 西庭灯 | 灯带 | 长廊灯 |
| --- | --- | --- | --- | --- | --- |
| **风和日丽** | `normal` | 风和日暖，庭院悠然 | ON | 暖白(255,210,150) 亮度40 常亮 | ON |
| **月白风清** | `night` | 月华如水，清风徐来 | ON | 月白(190,200,235) 亮度15 常亮 | ON |
| **清风徐来** | `hot` | 水波不兴，清凉消暑 | OFF | 冰蓝(70,185,250) 亮度45 呼吸 | OFF |
| **闲庭信步** | `relax` | 绿意盎然，信步闲游 | ON | 柔绿(65,210,95) 亮度28 呼吸 | OFF |
| **烟雨朦胧** | `rain` | 烟雨楼台，朦胧如画 | ON | 烟蓝(50,85,200) 亮度25 呼吸 | ON |
| **安全警戒** | `safe` | 全开高亮，安全第一 | ON | 纯白(255,255,255) 亮度100 警示闪烁 | ON |
| **万籁俱寂** | `eco` | 人静更深，万籁无声 | OFF | 关闭 | OFF |

### 安全警戒场景特殊联动

唯一直接操控继电器的场景：

- 强制关闭水泵和雾化器
- 确保安防总闸导通
- **关闭全局自动模式**（防止自动逻辑重新开启危险设备）

火灾传感器触发时会自动进入此场景。用户可随时说"打开自动模式"重新启用智能托管。

### 旧版名称兼容

以下名称仍然可用：`normal`、`夜间`、`闷热`、`休闲`、`雨天`、`安全`、`节能` 等。

## 自动模式

上电默认**关闭**，需用户主动开启。开启后全场景自动接管。

### 两层控制

| 状态字段 | 含义 |
| --- | --- |
| `auto_mode` | 全局自动总开关 |
| `auto_fan` | 通风扇是否允许被温湿度自动接管 |
| `auto_atomizer` | 雾化器是否允许被温湿度自动接管 |
| `auto_watering` | 水泵是否允许被土壤湿度自动接管 |
| `auto_rain_shelter` | 遮雨棚是否允许被雨滴传感器自动接管 |

一键开启：说"打开自动模式"或调用 `self.garden.set_auto_mode(enable=true)`。

### 自动控制阈值

来自 `slave/gardenslave/main/APP/Inc/sensor_config.h`：

| 参数 | 值 | 说明 |
| --- | --- | --- |
| `SOIL_WATER_START_PERCENT` | 45 | 土壤低于该湿度开始浇水 |
| `SOIL_WATER_STOP_PERCENT` | 60 | 土壤达到该湿度停止浇水 |
| `WATER_PUMP_MAX_RUN_MS` | 0 | 不限运行时间，仅靠湿度判断 |
| `WATER_PUMP_COOLDOWN_MS` | 0 | 停止后无冷却，干燥立即可重启 |
| `TEMP_COOLING_ON_C` | 32.0 | 高温自动降温启动 |
| `TEMP_COOLING_OFF_C` | 29.0 | 温度回落后允许关闭降温 |
| `HUMIDITY_DRY_ON_PERCENT` | 35.0 | 空气干燥自动开雾化 |
| `HUMIDITY_DRY_OFF_PERCENT` | 55.0 | 湿度恢复后关闭雾化 |
| `HUMIDITY_FAN_ON_PERCENT` | 80.0 | 高湿度自动开通风扇除湿 |
| `HUMIDITY_FAN_OFF_PERCENT` | 70.0 | 湿度回落后关闭通风扇 |
| `RAIN_THRESHOLD_PERCENT` | 60 | 雨滴百分比超过该值判定为下雨 |
| `FIRE_THRESHOLD_PERCENT` | 60 | 火焰百分比超过该值判定为火灾 |
| `FIRE_THRESHOLD_SAMPLES` | 3 | 连续超阈值次数(×500ms=1.5s)，防瞬时误报 |
| `FIRE_CLEAR_TIME_MS` | 15000 | 火灾解除后低于阈值持续时间 |

### 优先级规则

1. 火灾最高优先级：立即切断总闸，关闭水泵和雾化器，全开照明，灯带红色警示。
2. 小智手动控制优先：手动控制后对应单设备自动接管关闭。
3. 自动模式控制所有自动逻辑是否运行。
4. **场景与自动模式正交**：切换意境场景不影响自动模式控制的设备（安全警戒除外）。

## WS2812B 灯带效果

| effect | 值 | 说明 |
| --- | --- | --- |
| `solid` | 0 | 常亮 |
| `breathe` | 1 | 呼吸 |
| `rainbow` | 2 | 彩虹 |
| `alert` | 3 | 警示闪烁 |

灯带色序校准：`self.garden.calibrate_strip()` 依次显示红、绿、蓝，最后回到风和日丽。

## 火灾、求助、雨天事件

### 火灾熔断

采用 **ADC 电压阈值轮询** 替代传统中断模式，从根本上杜绝继电器 EMI 误报。

触发链路：

```text
火焰传感器 AO -> GPIO2(ADC1_CH1) 每 500ms 采样
  -> 电压下降 = 火焰靠近（百分比上升）
  -> fire_percent > FIRE_THRESHOLD_PERCENT(60%) 连续 3 次(1.5s)
  -> fire_status = FIRE_CONFIRMED
  -> GPIO17 = 0，安防总闸切断
  -> 水泵/雾化器关闭
  -> 三路照明打开
  -> WS2812B 红色警示闪烁
  -> 发布 xiaozhi/iot/emergency/{mac} + xiaozhi/iot/status/{mac}(场景=安全警戒)
  -> 主机 TTS 播报 + 蜂鸣器报警
```

自动解除：

```text
火焰移开 -> 电压回升 -> fire_percent <= 60% 持续 15 秒
  -> 自动恢复安防总闸导通、灯带风和日丽、发布场景通知
```

阈值配置见 `slave/gardenslave/main/APP/Inc/sensor_config.h`：`FIRE_THRESHOLD_PERCENT` 和 `FIRE_THRESHOLD_SAMPLES`。

手动解除：`self.garden.fire_reset()` — 恢复总闸高电平，灯带恢复风和日丽。

### 求助

GPIO48 下降沿触发：全开照明、灯带橙红警示、MQTT 紧急上报、主机播报。

### 雨天

雨滴连续超过阈值：展开遮雨棚到 90°、灯带切换到烟雨朦胧场景。

雨停连续干燥：**自动收回**遮雨棚到 0°、灯光恢复正常风和日丽。

## MCP 工具

默认启用的庭院高层工具：

| 工具 | 参数 | 用途 |
| --- | --- | --- |
| `self.garden.control` | `target`, `action` | 统一控制灯、通风扇、水泵、雾化器、总闸、遮雨棚 |
| `self.garden.set_strip_rgb` | `red`, `green`, `blue`, `brightness`, `effect` | 精确设置灯带颜色 |
| `self.garden.set_ambience_scene` | `scene` | 设置成语意境场景 |
| `self.garden.calibrate_strip` | 无 | 一键校准灯带红绿蓝色序 |
| `self.garden.emergency` | 无 | 触发火灾紧急响应 |
| `self.garden.fire_reset` | 无 | 解除火灾警报并恢复风和日丽 |
| `self.garden.get_status` | 无 | 获取设备状态 |
| `self.garden.get_sensors` | 无 | 获取传感器状态 |
| `self.garden.set_auto_mode` | `enable` | 一键开启/关闭全局自动模式 |
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
| `xiaozhi/iot/heartbeat/{mac}` | 从机 -> 主机/Web/App | 二进制 | 心跳(10s)，device_id=10 |
| `xiaozhi/iot/announce/{mac}` | 从机 -> 主机/Web/App | 二进制 | 上线声明，device_id=10 |
| `xiaozhi/iot/sensor/{mac}` | 从机 -> 主机/Web | JSON 或二进制 | 传感器状态 |
| `xiaozhi/iot/emergency/{mac}` | 从机 -> 主机/Web | JSON | 火灾、求助、雨天事件 |
| `xiaozhi/iot/status/{mac}` | 从机 -> 主机/Web/App | JSON | 场景切换通知 |
| `xiaozhi/garden/cmd` | Web/App -> 主机 | JSON | 高层庭院命令 |
| `xiaozhi/garden/status` | 主机 -> Web/App | JSON | 聚合设备状态 |
| `xiaozhi/garden/sensors` | 主机 -> Web/App | JSON | 聚合传感器状态 |
| `xiaozhi/garden/chat` | 主机 -> App | JSON | AI 对话实时转发（ASR/TTS 文本） |

### 命令码

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
| `IOT_CMD_SET_AUTO_MODE` | `0x35` | 全局自动模式 |
| `IOT_CMD_FIRE_RESET` | `0x36` | 火灾解除，恢复总闸和正常灯效 |
| `IOT_CMD_RAIN_ALERT` | `0x37` | 雨天事件 |
| `IOT_CMD_HELP_ALERT` | `0x38` | 求助事件 |
| `IOT_CMD_SET_STRIP_RGB` | `0x39` | 设置 WS2812B RGB/亮度/效果 |
| `IOT_CMD_SET_AMBIENCE_SCENE` | `0x3A` | 设置意境场景 |
| `IOT_CMD_SET_DEVICE_AUTO` | `0x3B` | 设置单设备自动接管 |

### App 直连从机指南

App 作为第二主机，可以直接通过 MQTT 控制从机，无需经过小智主机。

**订阅以下 Topic 获取状态**：

| 订阅 Topic | 格式 | 频率 | 用途 |
| --- | --- | --- | --- |
| `xiaozhi/iot/sensor/+` | JSON | 5s | 传感器原始数据 |
| `xiaozhi/iot/heartbeat/+` | 二进制 | 10s | 从机在线心跳 |
| `xiaozhi/iot/emergency/+` | JSON | 事件触发 | 火灾/求助/雨天告警 |
| `xiaozhi/garden/status` | JSON | 5s | 设备状态（需主机在线） |
| `xiaozhi/garden/sensors` | JSON | 5s | 传感器聚合（需主机在线） |
| `xiaozhi/garden/chat` | JSON | 实时 | AI 对话文本（用户语音识别 + AI 回复） |

**向从机发送二进制命令**：

Topic: `xiaozhi/iot/cmd/10`

Payload: `iot_command_packet_t` — 8 字节小端

```
Byte 0: command    (命令码)
Byte 1: device_id  (固定 10)
Byte 2: gpio_index (设备索引，见下方全局索引表)
Byte 3: value      (参数值)
Byte 4-7: reserved (扩展参数)
```

常用命令速查：

| 操作 | command | gpio_index | value | reserved |
| --- | --- | --- | --- | --- |
| 开西庭灯 | `0x11` | `0` | `1` | — |
| 关西庭灯 | `0x11` | `0` | `0` | — |
| 开长廊灯 | `0x11` | `2` | `1` | — |
| 开通风扇 | `0x12` | `0` | `1` | — |
| 开水泵 | `0x12` | `1` | `1` | — |
| 开雾化器 | `0x12` | `2` | `1` | — |
| 安防总闸导通 | `0x12` | `3` | `1` | — |
| 安防总闸切断 | `0x12` | `3` | `0` | — |
| 设置灯带 RGB | `0x39` | `1` | brightness | [R, G, B, effect] |
| 设置意境场景 | `0x3A` | `1` | scene_id | — |
| 展开遮雨棚 | `0x10` | `7` | `90` | — |
| 收回遮雨棚 | `0x10` | `7` | `0` | — |
| 开启自动模式 | `0x35` | `0` | `1` | — |
| 关闭自动模式 | `0x35` | `0` | `0` | — |
| 火灾解除 | `0x36` | `6` | `1` | — |

灯带 reserved 详解：`reserved[0]=R, reserved[1]=G, reserved[2]=B, reserved[3]=effect(0常亮/1呼吸/2彩虹/3警示)`

意境场景 scene_id：`0自定义 1月白风清 2清风徐来 3闲庭信步 4烟雨朦胧 5安全警戒 6风和日丽 7万籁俱寂`

### 庭院从机全局索引

| gpio_index | 设备 |
| --- | --- |
| 0 | 西庭灯 |
| 1 | 庭院 WS2812B 灯带 |
| 2 | 长廊灯 |
| 3 | 通风扇继电器 |
| 4 | 水泵继电器 |
| 5 | 雾化器继电器 |
| 6 | 安防总闸继电器 |
| 7 | 智能遮雨棚舵机 |

### 继电器局部索引

| relay_index | 设备 |
| --- | --- |
| 0 | 通风扇 |
| 1 | 水泵 |
| 2 | 雾化器 |
| 3 | 安防总闸 |

### 单设备自动接管 ID

| auto_id | target | 说明 |
| --- | --- | --- |
| 0 | `fan` | 通风扇温湿度自动接管 |
| 1 | `atomizer` | 雾化器温湿度自动接管 |
| 2 | `watering` | 水泵土壤湿度自动接管 |
| 3 | `rain_shelter` | 遮雨棚雨滴自动接管 |

### 意境场景枚举

| 枚举值 | 成语 | 英文键 |
| --- | --- | --- |
| 0 | 自定义 | `custom` |
| 1 | 月白风清 | `night` |
| 2 | 清风徐来 | `hot` |
| 3 | 闲庭信步 | `relax` |
| 4 | 烟雨朦胧 | `rain` |
| 5 | 安全警戒 | `safe` |
| 6 | 风和日丽 | `normal` |
| 7 | 万籁俱寂 | `eco` |

## JSON 高层命令

Topic：`xiaozhi/garden/cmd`

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

设置意境场景：

```json
{"device":"ambience","action":"scene","value":"风和日丽"}
{"device":"ambience","action":"scene","value":"月白风清"}
{"device":"ambience","action":"scene","value":"清风徐来"}
{"device":"ambience","action":"scene","value":"闲庭信步"}
{"device":"ambience","action":"scene","value":"烟雨朦胧"}
{"device":"ambience","action":"scene","value":"安全警戒"}
{"device":"ambience","action":"scene","value":"万籁俱寂"}
```

## 状态 JSON

Topic：`xiaozhi/garden/status`

```json
{
  "online": true,
  "expected": {
    "corridor_light": 1, "strip_light": 1, "veg_light": 1,
    "strip_rgb": [255, 210, 150], "strip_brightness": 40, "strip_effect": 0,
    "ambience_scene": "风和日丽",
    "fan": 0, "servo": 0, "water_pump": 0, "atomizer": 0, "safety_switch": 1,
    "auto_mode": false, "auto_fan": false, "auto_atomizer": false,
    "auto_watering": false, "auto_rain_shelter": false,
    "fire_status": 0
  },
  "actual": { "...": "字段同 expected" }
}
```

Topic：`xiaozhi/garden/sensors`

```json
{
  "soil": 48, "rain_percent": 12, "rain_adc_mv": 2600, "rain_state": "dry",
  "fire_status": 0, "help_status": 0,
  "temperature": 26.5, "humidity": 58.0
}
```

Topic：`xiaozhi/garden/chat`

用户语音识别结果（ASR）：

```json
{"role": "user", "text": "打开庭院灯", "ts": 1716800000}
```

AI 回复文本（TTS）：

```json
{"role": "assistant", "text": "好的，已为您打开庭院灯", "ts": 1716800001}
```

从机原始传感器上报（每 5 秒）+ INFO 日志（每 1 秒）：

```json
{"soil_mv":1450,"soil_percent":48,"rain_mv":2600,"rain_percent":12,
 "fire_mv":1950,"fire_percent":38,"fire_status":0,
 "rain_status":0,"help_status":0,"temperature":26.5,"humidity":58.0}
```

场景切换通知（实时）：

```json
{"type":"scene","scene_id":6,"scene_name":"风和日丽"}
```

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

烧录顺序：先从机，后主机。

## 现场演示脚本

1. 说"风和日丽"：全灯暖白，庭院悠然。
2. 说"月白风清"：月白色低亮，月下庭院的静谧感。
3. 说"清风徐来"：冰蓝灯带呼吸，如凉风拂面。
4. 说"闲庭信步"：柔绿灯带呼吸，绿意盎然。
5. 说"烟雨朦胧"：雾蓝灯带呼吸，仿佛细雨绵绵。
6. 说"打开自动模式"：全局自动接管，演示传感器驱动设备。
7. 模拟土壤干燥：水泵自动启动；湿润后自动停止。
8. 模拟下雨：遮雨棚展开到 90°，灯带切烟雨朦胧；停雨自动收回。
9. 说"万籁俱寂"：全部关闭，进入节能状态。
10. 触发火灾：总闸切断，红色警示，主机播报；说"解除火灾"恢复。

## 国奖展示亮点

- **关注点分离架构**：场景管氛围（灯光），自动模式管设备（继电器），正交不干扰。
- **成语意境系统**：用中国古典四字成语命名庭院场景，每个都有诗文出处和美学意境。
- **语音唤醒意境**：说成语就能切换庭院氛围，小智以成语应答，文化科技融合。
- **完整闭环**：自然语言 → MCP → MQTT → 从机执行器，端到端低延迟。
- **火灾安全本地闭环**：ADC 电压阈值轮询，抗继电器 EMI 干扰，不依赖网络和主机。
- **智能遮雨棚**：下雨自动展开 90°，雨停自动收回 0°并恢复正常场景。
- **多传感器联动**：土壤+水泵、温湿度+通风扇/雾化器、雨滴+舵机，六大传感器协同。
- **MQTT 统一总线**：Topic 和 JSON 接口已能支撑后续 App 或云端平台接入。
- **AI 对话实时同步**：App 订阅 `xiaozhi/garden/chat` 即可实时查看用户与 AI 的对话内容，支持 ASR 识别文本和 TTS 回复文本。
