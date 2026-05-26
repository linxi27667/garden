# MCP 工具说明文档

> 本文档详细说明小智 AI 与 ESP-NOW 从机设备之间的 MCP 工具调用、GPIO 控制和意图识别。

---

## 一、系统架构

```
┌─────────────────────────────────────────────────────────────────┐
│                      小智 AI (主机)                             │
│                                                                 │
│   ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐   │
│   │ iot_mcp_tool │  │alarm_mcp_tool│  │ medicine_mcp_tool  │   │
│   │ - Set_GPIO  │  │ - 报警控制   │  │ - 药盒控制          │   │
│   │ - Set_Servo │  └─────────────┘  └─────────────────────┘   │
│   └─────────────┘                                             │
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐  │
│   │              ESP-NOW 广播 / 单播                          │  │
│   └─────────────────────────────────────────────────────────┘  │
└───────────┬─────────────────┬─────────────────┬────────────────┘
            │                 │                 │
    ┌───────▼───────┐ ┌──────▼──────┐ ┌──────▼──────┐
    │  一楼从机      │ │  二楼从机    │ │  三楼从机    │
    │  (Firstfloor) │ │(Secondfloor)│ │ (Thirdfloor) │
    │  GPIO: 2,4,5  │ │GPIO: 2,4,5  │ │ GPIO: 2,4,5  │
    │  GPIO: 6,7,15 │ │GPIO: 6,7,15 │ │ GPIO: 6,7,15 │
    │  GPIO:18,21,40│ │GPIO:18,21,40│ │ GPIO:18,21,40│
    └───────────────┘ └─────────────┘ └─────────────┘
```

---

## 二、MCP 工具定义（主机端）

### 2.1 iot_mcp_tool（IoT 设备控制）

**文件**：`main/boards/custom-esp32s3-cam/iot_mcp_tool.cpp`

| MCP 工具函数 | 功能 | 参数 |
|--------------|------|------|
| `Iot_Mcp_Set_GPIO(device_name, gpio_name, value)` | 控制灯光/继电器 | device: 设备名, gpio: GPIO名, value: 0/1 |
| `Iot_Mcp_Set_Servo(device_name, angle)` | 控制舵机角度 | device: 设备名, angle: 0-180 |
| `Iot_Mcp_Get_GPIO(device_name, gpio_name)` | 读取GPIO状态 | 返回 0/1 |
| `Iot_Mcp_Get_Status_JSON(buffer, size)` | 获取所有设备状态 | JSON格式 |
| `Iot_Mcp_Discover()` | 发现新设备 | - |
| `Iot_Mcp_Add_Remote_Device(name, mac, gpio_count)` | 添加从机设备 | MAC地址 + GPIO数量 |

---

## 三、从机 GPIO 分配

### 3.1 一楼从机 (xiaozhi_slave_Firstfloor)

**设备名称**：`"一楼设备"`
**设备标识**：`DEVICE_NAME = "一楼设备"`

| 类别 | GPIO | 中文名称 | ESP-NOW 索引 |
|------|------|----------|-------------|
| 灯光 1 | GPIO 2 | 一楼卧室灯 | gpio_index 0 |
| 灯光 2 | GPIO 4 | 一楼客厅灯 | gpio_index 1 |
| 灯光 3 | GPIO 5 | 一楼西庭灯 | gpio_index 2 |
| 继电器 1 | GPIO 6 | 一楼电视电源 | gpio_index 3 |
| 继电器 2 | GPIO 7 | 一楼空调电源 | gpio_index 4 |
| 继电器 3 | GPIO 15 | 一楼热水器电源 | gpio_index 5 |
| 舵机 1 | GPIO 18 | 一楼窗户 | gpio_index 6 |
| 舵机 2 | GPIO 21 | 一楼窗帘 | gpio_index 7 |
| 舵机 3 | GPIO 40 | 一楼门锁 | gpio_index 8 |

---

### 3.2 二楼从机 (xiaozhi_slave_Secondfloor)

**设备名称**：`"二楼设备"`
**设备标识**：`DEVICE_NAME = "二楼设备"`

| 类别 | GPIO | 中文名称 | ESP-NOW 索引 |
|------|------|----------|-------------|
| 灯光 1 | GPIO 2 | 二楼浴室灯 | gpio_index 0 |
| 灯光 2 | GPIO 4 | 二楼客厅灯 | gpio_index 1 |
| 灯光 3 | GPIO 5 | 二楼卧室灯 | gpio_index 2 |
| 继电器 1 | GPIO 6 | 二楼电视电源 | gpio_index 3 |
| 继电器 2 | GPIO 7 | 二楼空调电源 | gpio_index 4 |
| 继电器 3 | GPIO 15 | 二楼热水器电源 | gpio_index 5 |
| 舵机 1 | GPIO 18 | 二楼窗户 | gpio_index 6 |
| 舵机 2 | GPIO 21 | 二楼窗帘 | gpio_index 7 |
| 舵机 3 | GPIO 40 | 二楼门锁 | gpio_index 8 |

---

### 3.3 三楼从机 (xiaozhi_slave_Thirdfloor)

**设备名称**：`"三楼设备"`
**设备标识**：`DEVICE_NAME = "三楼设备"`

| 类别 | GPIO | 中文名称 | ESP-NOW 索引 |
|------|------|----------|-------------|
| 灯光 1 | GPIO 2 | 三楼卧室灯 | gpio_index 0 |
| 灯光 2 | GPIO 4 | 三楼客厅灯 | gpio_index 1 |
| 灯光 3 | GPIO 5 | 三楼西庭灯 | gpio_index 2 |
| 继电器 1 | GPIO 6 | 三楼电视电源 | gpio_index 3 |
| 继电器 2 | GPIO 7 | 三楼空调电源 | gpio_index 4 |
| 继电器 3 | GPIO 15 | 三楼热水器电源 | gpio_index 5 |
| 舵机 1 | GPIO 18 | 三楼窗户 | gpio_index 6 |
| 舵机 2 | GPIO 21 | 三楼窗帘 | gpio_index 7 |
| 舵机 3 | GPIO 40 | 三楼阳台门 | gpio_index 8 |

**传感器（本地 ADC，无 ESP-NOW）**：

| 传感器 | GPIO/通道 | 说明 |
|--------|-----------|------|
| 烟雾传感器 | ADC1_CH0 (GPIO 1) | 模拟输入 |
| 雨滴传感器 | ADC1_CH2 (GPIO 3) | 模拟输入 |
| 求助按键 | GPIO 48 | 外部上拉 |

**ASR 语音告警**：

| 功能 | GPIO | 协议 |
|------|------|------|
| ASR UART TX | GPIO 17 | 单字节命令：0x01=火灾, 0x02=求助 |

---

## 四、AI 意图识别映射

### 4.1 灯光控制

| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 开灯 | `Iot_Mcp_Set_GPIO("X楼设备", "灯光X", 1)` | "打开一楼卧室灯" |
| 关灯 | `Iot_Mcp_Set_GPIO("X楼设备", "灯光X", 0)` | "关闭二楼客厅灯" |
| 全开灯 | `Iot_Mcp_Broadcast_Lights_On()` | "打开所有灯" |
| 全关灯 | `Iot_Mcp_Broadcast_Lights_Off()` | "关闭所有灯" |

### 4.2 继电器控制

| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 打开电源 | `Iot_Mcp_Set_GPIO("X楼设备", "电源X", 1)` | "打开一楼电视电源" |
| 关闭电源 | `Iot_Mcp_Set_GPIO("X楼设备", "电源X", 0)` | "关闭二楼空调电源" |

### 4.3 舵机控制

| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 打开窗户 | `Iot_Mcp_Set_Servo("X楼窗户", 180)` | "打开一楼窗户" |
| 关闭窗户 | `Iot_Mcp_Set_Servo("X楼窗户", 0)` | "关闭三楼窗户" |
| 打开窗帘 | `Iot_Mcp_Set_Servo("X楼窗帘", 180)` | "拉开二楼窗帘" |
| 关闭窗帘 | `Iot_Mcp_Set_Servo("X楼窗帘", 0)` | "拉上三楼窗帘" |
| 开门 | `Iot_Mcp_Set_Servo("X楼门锁", 180)` | "打开一楼门锁" |
| 关门 | `Iot_Mcp_Set_Servo("X楼门锁", 0)` | "关闭二楼门锁" |

### 4.4 一键控制

| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 全开 | `Iot_Mcp_Broadcast_All_On()` | "打开所有设备" |
| 全关 | `Iot_Mcp_Broadcast_All_Off()` | "关闭所有设备" |

### 4.5 智慧庭院氛围灯

| AI 意图 | MCP 工具 | 示例话术 |
|---------|----------|----------|
| 通用设备控制 | `self.garden.control(target, action)` | "打开雾化器"、"切断安防总闸"、"展开遮雨棚" |
| 一键灯光/设备 | `self.garden.all(scope, action)` | "打开所有灯"、"关闭所有设备" |
| 精确调色 | `self.garden.set_strip_rgb(red, green, blue, brightness, effect)` | "把庭院灯带调成浅蓝色，亮度 50%" |
| 夜间模式 | `self.garden.set_ambience_scene("night")` | "进入夜间模式" |
| 闷热降温 | `self.garden.set_ambience_scene("hot")` | "我有点闷热，帮我调节一下" |
| 休闲氛围 | `self.garden.set_ambience_scene("relax")` | "来点放松的庭院氛围" |
| 聚会氛围 | `self.garden.set_ambience_scene("party")` | "打开聚会模式" |
| 雨天氛围 | `self.garden.set_ambience_scene("rain")` | "切到雨天氛围" |
| 安全照明 | `self.garden.set_ambience_scene("safe")` | "打开安全照明" |
| 恢复正常 | `self.garden.set_ambience_scene("normal")` | "恢复正常灯光" |
| 单设备自动 | `self.garden.set_device_auto(target, enable)` | "恢复自动浇水"、"关闭遮雨棚自动接管" |
| 灯带校准 | `self.garden.calibrate_strip()` | "校准一下灯带颜色" |
| 火灾应急 | `self.garden.emergency()` | "触发火灾应急" |
| 解除火灾 | `self.garden.fire_reset()` | "火灾解除了，恢复正常" |

WS2812B 灯带接在庭院从机 GPIO12，使用 MQTT `IOT_CMD_SET_STRIP_RGB(0x39)` 控制 RGB/亮度/效果，原 `strip_light` 开关语义继续保留。

安防总闸硬件语义：GPIO17 高电平表示继电器导通、庭院负载正常供电；GPIO17 低电平表示火灾熔断、切断庭院负载。上电默认应为高电平，只有火灾或明确切断总闸时才拉低。

为降低大模型选错工具的概率，默认关闭底层 `self.iot.*` 与过细的 `self.garden.set_fan/set_water_pump/set_atomizer/set_safety_switch` 等工具，统一使用 `self.garden.control` 做常规执行器控制。需要重新打开底层工具时，可在编译参数里设置 `ENABLE_LEGACY_IOT_MCP_TOOLS=1` 或 `ENABLE_GARDEN_FINE_GRAINED_MCP_TOOLS=1`。

---

## 五、ESP-NOW 协议格式

### 5.1 命令包结构

```c
typedef struct {
    uint8_t command;     // 命令码
    uint8_t device_id;  // 设备ID
    uint8_t gpio_index;  // GPIO索引 (0-8)
    uint8_t value;       // 值 (0/1 或 角度)
    uint8_t reserved[4]; // 保留
} iot_command_packet_t;
```

### 5.2 命令码定义

| 命令码 | 名称 | 说明 |
|--------|------|------|
| 0x01 | `IOT_CMD_SET_GPIO` | 设置 GPIO |
| 0x02 | `IOT_CMD_GET_GPIO` | 读取 GPIO |
| 0x03 | `IOT_CMD_GET_ALL_GPIO` | 读取所有 GPIO |
| 0x04 | `IOT_CMD_HEARTBEAT` | 心跳包 |
| 0x05 | `IOT_CMD_RESPONSE` | 响应 |
| 0x06 | `IOT_CMD_DISCOVER` | 发现设备 |
| 0x07 | `IOT_CMD_ANNOUNCE` | 设备声明 |
| 0x08 | `IOT_CMD_ANNOUNCE_V2` | 设备声明V2 |
| 0x10 | `IOT_CMD_SET_SERVO` | 设置舵机角度 |
| 0x11 | `IOT_CMD_SET_LIGHT` | 设置灯光 |
| 0x12 | `IOT_CMD_SET_RELAY` | 设置继电器 |
| 0x30 | `IOT_CMD_BROADCAST_ALL_OFF` | 广播全关 |
| 0x31 | `IOT_CMD_BROADCAST_ALL_ON` | 广播全开 |
| 0x32 | `IOT_CMD_BROADCAST_LIGHTS_OFF` | 广播关灯 |
| 0x33 | `IOT_CMD_BROADCAST_LIGHTS_ON` | 广播开灯 |
| 0x39 | `IOT_CMD_SET_STRIP_RGB` | 设置 WS2812B 灯带 RGB/亮度/效果 |
| 0x3A | `IOT_CMD_SET_AMBIENCE_SCENE` | 设置庭院氛围场景 |

---

## 六、三楼特殊功能

### 6.1 传感器检测（自动触发，无需 AI）

| 触发条件 | 动作 |
|----------|------|
| 雨滴 < 1320mV (大雨) | 自动关闭窗帘(舵机2→0度) |
| 烟雾 >= 1500mV | ASR 播报火灾警报 (0x01) |
| 按下求助按键(GPIO48) | ASR 播报紧急求助 (0x02) |
| ESP-NOW收到0x34命令 | ASR 播报紧急求救 (0x03) |

### 6.2 ASR UART 命令

| 命令 | 含义 | 触发条件 |
|------|------|----------|
| 0x01 | 火灾报警 - 播报"检测到火灾，请注意" | 烟雾>=60% |
| 0x02 | 求助报警 - 播报"紧急求助，請立即协助" | GPIO48按下 |
| 0x03 | 求救报警 - 播报"救命啊，请救救我" | ESP-NOW远程触发/AI呼唤"救命" |

---

## 七、GPIO 索引速查表

| gpio_index | 设备 | 一楼 | 二楼 | 三楼 |
|------------|------|------|------|------|
| 0 | 灯光1 | 卧室灯 | 浴室灯 | 三楼卧室灯 |
| 1 | 灯光2 | 客厅灯 | 客厅灯 | 三楼客厅灯 |
| 2 | 灯光3 | 西庭灯 | 卧室灯 | 三楼西庭灯 |
| 3 | 继电器1 | 电视电源 | 电视电源 | 三楼电视电源 |
| 4 | 继电器2 | 空调电源 | 空调电源 | 三楼空调电源 |
| 5 | 继电器3 | 热水器电源 | 热水器电源 | 三楼热水器电源 |
| 6 | 舵机1 | 大门 | 遮雨棚 | **左天窗** |
| 7 | 舵机2 | 窗户 | 窗帘 | **右天窗** |
| 8 | 舵机3 | 门锁 | 阳台门 | **三楼遮雨棚** |

---

## 八、舵机角度枚举

| 枚举值 | 实际角度 | 说明 |
|--------|----------|------|
| SERVO_0 | 0° | 完全关闭 |
| SERVO_45 | 45° | 半开 |
| SERVO_90 | 90° | 中间 |
| SERVO_135 | 135° | 大部分开启 |
| SERVO_180 | 180° | 完全打开 |

---

## 九、场景化 MCP 工具（新增）

### 9.1 回家场景 (`Iot_Mcp_Scenario_Homecoming`)
| 触发意图 | 执行动作 |
|----------|----------|
| "我回家了" / "回家" | 打开一楼大门(舵机2→180°) + 打开所有楼层灯光 |

### 9.2 晚安场景 (`Iot_Mcp_Scenario_GoodNight`)
| 触发意图 | 执行动作 |
|----------|----------|
| "晚安" / "我要睡觉了" / "祝我好梦" | 关闭所有楼层灯光（保留其他设备） |

### 9.3 求救场景 (`Iot_Mcp_Scenario_Emergency`)
| 触发意图 | 执行动作 |
|----------|----------|
| "救命" / "救救我" / " SOS" | ESP-NOW向三楼发送0x03命令 → ASR PRO播报求救语音 |

### 9.4 雨天遮雨场景 (`Iot_Mcp_Scenario_Rain_Deploy_Shelter`)
| 触发意图 | 执行动作 |
|----------|----------|
| "下雨了" / "展开遮雨棚" | 庭院遮雨棚舵机→180°，灯带切到雨天冷蓝呼吸 |

### 9.5 通风场景 (`Iot_Mcp_Scenario_Ventilate`)
| 触发意图 | 执行动作 |
|----------|----------|
| "我好闷" / "打开通风" | 三楼左天窗+右天窗打开指定角度 + 打开二楼风扇 |

### 9.6 舵机精确角度控制 (`Iot_Mcp_Control_Servo_Angle`)
| 触发意图 | 执行动作 |
|----------|----------|
| "把左天窗调到45度" / "打开天窗一半" | 精确设置舵机角度(0-180°) |

---

## 十、AI 意图识别完整映射表

### 10.1 灯光控制
| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 开X楼灯 | `Iot_Mcp_Set_GPIO("X楼设备", "灯光X", 1)` | "打开一楼客厅灯" |
| 关X楼灯 | `Iot_Mcp_Set_GPIO("X楼设备", "灯光X", 0)` | "关闭三楼西庭灯" |
| 全开灯 | `Iot_Mcp_Broadcast_Lights_On()` | "打开所有灯" |
| 全关灯 | `Iot_Mcp_Broadcast_Lights_Off()` | "关闭所有灯" |

### 10.2 舵机/天窗控制
| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 打开天窗 | `Iot_Mcp_Control_Servo_Angle("三楼设备", 0, 180)` | "打开左天窗" |
| 关闭天窗 | `Iot_Mcp_Control_Servo_Angle("三楼设备", 0, 0)` | "关闭左天窗" |
| 展开遮雨棚 | `self.garden.control("rain_shelter", "deploy")` | "展开遮雨棚" |
| 收回遮雨棚 | `self.garden.control("rain_shelter", "retract")` | "收回遮雨棚" |

### 10.3 继电器/风扇控制
| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 打开风扇 | `Iot_Mcp_Set_GPIO("二楼设备", "二楼空调电源", 1)` | "打开风扇" |
| 我好热 | → 询问是否开风扇 | "小智我好热啊" |
| 我好闷 | → 询问是否通风 | "小智我好闷呀" |

### 10.4 场景化控制
| AI 意图 | 调用函数 | 示例话术 |
|---------|----------|----------|
| 回家 | `Iot_Mcp_Scenario_Homecoming()` | "我回家了" |
| 晚安 | `Iot_Mcp_Scenario_GoodNight()` | "晚安小智" |
| 求救 | `Iot_Mcp_Scenario_Emergency()` | "救命" |

---

## 十一、历史变更

| 日期 | 变更内容 |
|------|----------|
| 2026-05-26 | 庭院舵机语义升级为智能遮雨棚，新增单设备自动接管和灯带校准 |
| 2026-03-30 | 新增场景化MCP工具(回家/晚安/求救/雨天遮雨/通风) |
| 2026-03-30 | 三楼舵机重命名:窗户→左天窗,窗帘→右天窗,阳台门→三楼遮雨棚 |
| 2026-03-30 | ESP-NOW新增0x03求救命令支持，触发ASR PRO求救语音 |
| 2026-03-29 | 添加三楼传感器(烟雾/雨滴/求助) + ASR UART |
| 2026-03-29 | ASR UART 从 GPIO43 改为 GPIO17 |
| 2026-03-27 | 修复舵机 gpio_index 验证逻辑 |
| 2026-03-26 | 统一三个楼层 GPIO 分配 |
