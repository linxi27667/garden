# 智慧庭院从机 (Garden Slave)

智慧庭院 IoT 从机设备，基于 ESP32，通过 MQTT 协议与主机（小智 AI 助手）通信，实现设备控制、传感器采集、紧急事件上报。

## 功能特性

### 设备控制
- 3 路灯光控制（西庭灯 / 庭院灯带 / 长廊灯）
- 4 路继电器控制（风扇 / 水泵 / 雾化器 / 安防总闸）
- 1 路舵机控制（晾衣架：晾/收）
- 全开/全关灯光、全部设备关闭

### 传感器采集
- 土壤湿度（ADC 采集，GPIO1）
- 光照强度（ADC 采集，GPIO3）
- 雨滴检测（光照阈值判断）
- 火灾检测（GPIO10 中断，烟雾传感器）
- 求助按钮（GPIO11 中断）
- 温湿度（DHT11，GPIO8）
- 5 秒周期 JSON 上报至 MQTT

### 智能自动化
- 自动模式（可语音开关）
  - 雨天自动收衣 + MQTT 通知主机
  - 高温自动开风扇 + 雾化器
  - 低温自动关风扇
  - 低湿度自动开雾化器
  - 高湿度自动开风扇除湿

### 紧急事件链路
- **火灾**：GPIO 中断 → 本地响应（切总闸/关水泵/全开灯）→ MQTT 紧急上报 → ASR 语音播报 → 主机唤醒小智 → 蜂鸣器报警 → 语音播报"火灾警报"
- **求助**：GPIO 中断 → 全开灯 → MQTT 紧急上报 → ASR 语音播报 → 主机唤醒小智 → 蜂鸣器报警 → 语音播报"紧急求助"
- **火灾解除**：主机发送 `IOT_CMD_FIRE_RESET` → 从机恢复正常状态

### 通信机制
- MQTT 协议通信（非 ESP-NOW）
- 自动发现主机并注册（Announce V2）
- 心跳保活机制
- 设备在线状态监测

## 硬件配置

| GPIO | 名称 | 功能 | 说明 |
|------|------|------|------|
| GPIO2 | corridor_light | 西庭灯 | 数字输出 |
| GPIO4 | strip_light | 庭院灯带 | 数字输出 |
| GPIO5 | veg_light | 长廊灯 | 数字输出 |
| GPIO6 | fan_relay | 风扇继电器 | 数字输出 |
| GPIO7 | water_pump | 水泵继电器 | 数字输出 |
| GPIO15 | atomizer | 雾化器继电器 | 数字输出 |
| GPIO16 | safety_switch | 安防总闸 | NC常闭端，高电平断开 |
| GPIO18 | clothes_servo | 晾衣架舵机 | LEDC PWM |
| GPIO10 | fire_sensor | 火灾传感器 | 下降沿中断 |
| GPIO11 | help_button | 求助按钮 | 下降沿中断 |
| GPIO8 | dht11 | 温湿度传感器 | 单总线 |
| GPIO1 | soil_adc | 土壤湿度 | ADC1_CH0 |
| GPIO3 | light_adc | 光照强度 | ADC1_CH2 |
| GPIO17 | asr_tx | ASR语音模块 | UART1 TX |

## WiFi 配置

修改 `main/APP/Src/app_wifi.c`：

```c
#define ROUTER_SSID      "你的WiFi名称"
#define ROUTER_PASSWORD  "你的WiFi密码"
```

## 编译和烧录

```bash
cd gardenslave
idf.py build
idf.py -p COM3 flash monitor
```

## MQTT 通信协议

### 主题结构

| 主题 | 方向 | 说明 |
|------|------|------|
| `xiaozhi/iot/cmd/broadcast` | 主机→从机 | 广播命令 |
| `xiaozhi/iot/cmd/10` | 主机→从机 | 庭院从机专属命令 |
| `xiaozhi/iot/resp/<mac>` | 从机→主机 | 命令响应 |
| `xiaozhi/iot/announce/<mac>` | 从机→主机 | 设备公告 |
| `xiaozhi/iot/heartbeat/<mac>` | 从机→主机 | 心跳 |
| `xiaozhi/iot/sensor/<mac>` | 从机→主机 | 传感器数据（JSON） |
| `xiaozhi/iot/emergency/<mac>` | 从机→主机 | 紧急事件 |

### 命令码

| 命令 | 值 | 说明 |
|------|-----|------|
| IOT_CMD_SET_GPIO | 0x01 | 设置 GPIO |
| IOT_CMD_SET_LIGHT | 0x11 | 设置灯光 |
| IOT_CMD_SET_RELAY | 0x12 | 设置继电器 |
| IOT_CMD_SET_SERVO | 0x10 | 设置舵机角度 |
| IOT_CMD_SET_AUTO_MODE | 0x35 | 设置自动模式 |
| IOT_CMD_FIRE_RESET | 0x36 | 火灾状态重置 |
| IOT_CMD_EMERGENCY | 0x34 | 紧急响应 |
| IOT_CMD_DISCOVER | 0x06 | 发现设备 |
| IOT_CMD_BROADCAST_ALL_OFF | 0x30 | 全部关闭 |
| IOT_CMD_BROADCAST_ALL_ON | 0x31 | 全部开启 |
| IOT_CMD_BROADCAST_LIGHTS_OFF | 0x32 | 灯光全关 |
| IOT_CMD_BROADCAST_LIGHTS_ON | 0x33 | 灯光全开 |

### 传感器数据格式（JSON）

```json
{
  "soil_mv": 1234,
  "light_mv": 2345,
  "fire_status": 0,
  "rain_status": 0,
  "help_status": 0,
  "temperature": 25.5,
  "humidity": 60.0
}
```

### 紧急事件格式（JSON）

```json
{
  "type": "emergency",
  "device_id": "AABBCCDDEEFF",
  "emergency_type": 0,
  "detail": "Fire detected by smoke sensor"
}
```

## 项目结构

```
gardenslave/
├── CMakeLists.txt
├── sdkconfig.defaults
├── README.md
└── main/
    ├── iot_slave_main.c          # 入口
    ├── APP/                       # 应用层
    │   ├── Inc/
    │   │   ├── iot_control_task.h # 设备控制+标志位定义
    │   │   ├── mqtt_receive.h     # MQTT接收+紧急上报
    │   │   ├── mqtt_heartbeat.h   # 心跳
    │   │   ├── sensor_adc_task.h  # 传感器采集
    │   │   ├── sensor_config.h    # 传感器配置
    │   │   └── app_asr_uart.h     # ASR语音模块
    │   └── Src/
    │       ├── iot_control_task.c # Flag-driven设备刷新+中断处理
    │       ├── mqtt_receive.c     # MQTT命令解析+紧急上报
    │       ├── mqtt_heartbeat.c   # 心跳发送
    │       ├── sensor_adc_task.c  # ADC+DHT11采集+自动模式
    │       └── app_asr_uart.c     # ASR语音播报
    └── BSP/                       # 底层驱动
        ├── Inc/
        │   ├── bsp_asr_uart.h     # ASR BSP抽象
        │   └── bsp_wifi.h
        └── Src/
            ├── bsp_asr_uart.c     # ASR BSP实现
            └── bsp_wifi.c
```

## 注意事项

1. **WiFi必须相同**：从机和主机必须连接同一个WiFi路由器
2. **MQTT Broker**：默认使用 `8.134.167.240:1883`，可修改
3. **安防总闸**：使用NC常闭端，高电平吸合时切断庭院总电源
4. **舵机供电**：舵机需要独立5V供电，ESP32的3.3V不足
5. **DHT11时序**：读取时阻塞CPU约20ms，已绑定到Core 1
6. **火灾解除**：必须由主机发送 `IOT_CMD_FIRE_RESET` 才能恢复正常
