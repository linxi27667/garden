# MQTT IoT 架构完整说明

## 一、系统总览

### 1.1 架构概览

```
                    ┌─────────────────────────────────────┐
                    │      MQTT Broker (WiFi路由器)        │
                    │   broker.emqx.io:1883                │
                    │   或局域网自建 Mosquitto/EMQX        │
                    └──────────┬──────────────────────────┘
                               │
          ┌────────────────────┼─────────────────────────────┐
          │                    │                              │
    ┌─────┴──────┐      ┌──────┴──────┐              ┌───────┴──────┐
    │  主机(S3)   │      │  从机(一楼)  │              │  从机(二楼)  │
    │            │      │             │              │             │
    │ AI对话      │      │ 3灯+3继电器 │              │ 3灯+3继电器 │
    │ IoT控制     │      │ +3舵机      │              │ +3舵机      │
    │            │      │             │              │             │
    │ 发布命令    │─────→│ 接收命令    │              │ 接收命令    │
    │ 订阅响应    │←─────│ 发布响应    │              │ 发布响应    │
    └─────┬──────┘      └──────┬──────┘              └──────┬──────┘
          │                    │                              │
          │              ┌─────┴──────┐
          │              │  从机(三楼) │
          │              │             │
          │              │ 3灯+3继电器 │
          └──────────────│ +3舵机     │
                         │ +传感器    │
                         └────────────┘
```

### 1.2 核心设计原则

1. **传输层与应用层分离**：MQTT只负责传输，命令解析逻辑（`Process_Command`）完全不变
2. **单向控制流**：主机→从机（命令），从机→主机（响应/心跳/声明）
3. **广播+单播混合**：所有从机订阅广播Topic，同时订阅各自的单播Topic
4. **设备自动发现**：从机上线时自动发布ANNOUNCE（含MAC+device_id），主机无需预配
5. **断线自动重连**：ESP-IDF MQTT客户端内置重连，重连后自动重新订阅
6. **Topic携带身份**：所有从机→主机的Topic均包含MAC地址，主机从Topic解析来源

---

## 二、通信协议

### 2.1 Topic路由表

| Topic | 发布者 | 订阅者 | QoS | 用途 |
|-------|--------|--------|-----|------|
| `xiaozhi/iot/cmd/broadcast` | 主机 | 所有从机 | 1 | 全局广播命令 |
| `xiaozhi/iot/cmd/1` | 主机 | 一楼从机 | 1 | 单播命令到一楼 |
| `xiaozhi/iot/cmd/2` | 主机 | 二楼从机 | 1 | 单播命令到二楼 |
| `xiaozhi/iot/cmd/3` | 主机 | 三楼从机 | 1 | 单播命令到三楼 |
| `xiaozhi/iot/resp/{mac}` | 从机 | 主机（通配符+） | 0 | 从机执行响应 |
| `xiaozhi/iot/heartbeat/{mac}` | 从机 | 主机（通配符+） | 0 | 心跳保活（30s） |
| `xiaozhi/iot/announce/{mac}` | 从机 | 主机（通配符+） | 1 | 设备上线声明 |

> **关键设计**：`announce/{mac}` 而非 `announce`，确保主机能从Topic解析出设备MAC地址，避免多设备碰撞。

### 2.2 Payload格式

#### 命令包 `iot_command_packet_t`（8字节）

```
偏移  0       1         2          3        4-7
     [command][device_id][gpio_index][value]  [reserved]
```

#### 心跳包 `iot_heartbeat_packet_t`（17字节）

```
偏移  0       1         2          3        4-16
     [command][device_id][gpio_index][value]  [mac_str(13字节)]
```

#### 声明包 `iot_announce_v2_packet_t`（28字节）

```
偏移  0       1         2          3        4-27
     [command][device_id][gpio_index][value]  [device_name(24字节)]
```

> **device_id字段**：从机在Announce包中设置正确的device_id（1=一楼, 2=二楼, 3=三楼），主机使用该值而非发现顺序来路由命令。

### 2.3 命令码表

| 命令码 | 十六进制 | 方向 | 说明 |
|--------|----------|------|------|
| `IOT_CMD_SET_GPIO` | 0x01 | 主机→从机 | 设置GPIO（通用） |
| `IOT_CMD_SET_LIGHT` | 0x11 | 主机→从机 | 设置灯光 |
| `IOT_CMD_SET_RELAY` | 0x12 | 主机→从机 | 设置继电器 |
| `IOT_CMD_SET_SERVO` | 0x10 | 主机→从机 | 设置舵机角度 |
| `IOT_CMD_HEARTBEAT` | 0x04 | 双向 | 心跳 |
| `IOT_CMD_RESPONSE` | 0x05 | 从机→主机 | 命令执行响应 |
| `IOT_CMD_DISCOVER` | 0x06 | 主机→从机 | 发现设备 |
| `IOT_CMD_ANNOUNCE` | 0x07 | 从机→主机 | 设备声明（旧版） |
| `IOT_CMD_ANNOUNCE_V2` | 0x08 | 从机→主机 | 设备声明（新版） |
| `IOT_CMD_OPEN_MEDICINE_BOX` | 0x20 | 主机→从机 | 打开药盒 |
| `IOT_CMD_BROADCAST_ALL_OFF` | 0x30 | 广播 | 全关 |
| `IOT_CMD_BROADCAST_ALL_ON` | 0x31 | 广播 | 全开 |
| `IOT_CMD_BROADCAST_LIGHTS_OFF` | 0x32 | 广播 | 关灯 |
| `IOT_CMD_BROADCAST_LIGHTS_ON` | 0x33 | 广播 | 开灯 |
| `IOT_CMD_EMERGENCY` | 0x34 | 广播 | 紧急模式 |

---

## 三、代码架构

### 3.1 目录结构

```
xiaozhi-esp32-main/
├── shared/                          # 共享代码（所有设备共用）
│   └── mqtt_iot_protocol.h          # MQTT协议定义（Topic/命令码/数据结构）
│
├── main/                            # 主机 (ESP32-S3)
│   ├── APP/Src/
│   │   └── iot_controller.c         # IoT应用层：初始化、场景控制、超时检测
│   ├── BSP/Src/
│   │   └── bsp_iot_controller.c     # IoT BSP层：命令构造、MQTT发送、设备管理
│   └── BSP/Inc/
│       └── bsp_iot_controller.h     # IoT BSP接口（IOT_MAX_REMOTE_GPIOS=16）
│
├── xiaozhi_slave_Firstfloor/        # 一楼从机 (ESP32-S3)
│   └── main/
│       ├── APP/Src/
│       │   ├── mqtt_receive.c       # MQTT接收+命令解析+导出Client/MAC
│       │   ├── mqtt_heartbeat.c     # MQTT心跳（通过getter获取Client）
│       │   └── iot_control_task.c   # 设备控制：标志位刷新+GPIO操作
│       ├── APP/Inc/
│       │   └── iot_control_task.h   # 设备枚举+标志位定义
│       └── main/
│           └── iot_slave_main.c     # 主入口
│
├── xiaozhi_slave_Secondfloor/       # 二楼从机 (同架构)
├── xiaozhi_slave_Thirdfloor/        # 三楼从机 (同架构，多了传感器)
│
└── lvgl_prj/                        # P4面板 (暂未改造)
```

### 3.2 主机软件架构

```
iot_controller.c (APP层)
    │
    ├── App_IOT_System_Init()           # 系统初始化
    │   ├── IOT_Controller_Init()       # 初始化控制器结构体
    │   ├── HW_MQTT_Init()             # 初始化MQTT客户端
    │   └── App_IOT_Discover_Devices()  # 发送发现命令
    │
    ├── mqtt_event_handler()            # MQTT事件回调
    │   ├── CONNECTED → 重新订阅3个Topic（断线重连保护）
    │   │   ├── xiaozhi/iot/resp/+
    │   │   ├── xiaozhi/iot/heartbeat/+
    │   │   └── xiaozhi/iot/announce/+
    │   ├── DISCONNECTED → 日志
    │   ├── DATA → HW_MQTT_Recv_Callback()
    │   └── ERROR → 日志
    │
    ├── HW_MQTT_Recv_Callback()         # 接收数据处理
    │   ├── 拷贝topic到栈缓冲区并null-terminate（安全处理）
    │   ├── 从Topic解析MAC地址（heartbeat/resp/announce）
    │   ├── 心跳 → IOT_Update_Heartbeat()
    │   └── 响应/声明 → IOT_Process_Remote_Response()
    │
    ├── App_IOT_Set_Light/Relay/Servo() # 设备控制
    ├── App_IOT_Broadcast_*()           # 广播控制
    └── App_IOT_Scenario_*()            # 场景控制
```

```
bsp_iot_controller.c (BSP层)
    │
    ├── IOT_Get_Device_Id_By_Name()    # 根据设备名推导device_id
    │   └── "一楼*"→1, "二楼*"→2, "三楼*"→3
    │
    ├── IOT_Resolve_Alias()            # 设备名别名解析
    │   └── "主卧"→"二楼设备", "阳台灯"→"三楼设备" 等
    │
    ├── IOT_Add_Remote_Device()        # 注册远程设备
    │   └── device_id = IOT_Get_Device_Id_By_Name(name)
    │       （基于设备身份而非发现顺序）
    │
    ├── IOT_Set_Remote_GPIO()          # 构造命令包 + MQTT Publish
    │   └── Topic: xiaozhi/iot/cmd/{device_id}
    │
    ├── IOT_Process_Remote_Response()  # 处理从机响应
    │   ├── MAC匹配已有设备 → 更新状态
    │   ├── ANNOUNCE_V2 → 更新设备信息
    │   └── 新设备 → IOT_Add_Remote_Device()
    │
    ├── IOT_Update_Heartbeat()         # 通过MAC匹配更新心跳时间
    │
    └── IOT_Broadcast_*/IOT_Set_*/IOT_Send_Heartbeat()
```

### 3.3 从机软件架构

```
mqtt_receive.c (MQTT接收层)
    │
    ├── MQTT_Receive_Init()            # 连接Broker，读取MAC
    │   ├── esp_read_mac() → s_mac_str
    │   ├── esp_mqtt_client_init() + register_event + start
    │   └── 导出 MQTT_Get_Client() / MQTT_Get_MAC_Str()
    │
    ├── mqtt_event_handler()           # MQTT事件回调
    │   ├── CONNECTED → 重新订阅 + 发布ANNOUNCE
    │   │   ├── 订阅 xiaozhi/iot/cmd/broadcast
    │   │   ├── 订阅 xiaozhi/iot/cmd/{device_id}
    │   │   └── Send_Announce() → xiaozhi/iot/announce/{mac}
    │   ├── DISCONNECTED → 日志
    │   └── DATA → Process_Command()
    │
    ├── Process_Command()              # 命令解析（只更新标志位）
    │   ├── IOT_CMD_SET_GPIO → g_device_flags.light[] / relay[]
    │   ├── IOT_CMD_SET_SERVO → g_device_flags.servo[]
    │   │   └── 三楼: 角度无效时不发响应
    │   └── IOT_CMD_BROADCAST_* → 批量更新标志位
    │
    ├── Send_Response()                # 发送响应（NULL安全）
    │   └── Publish → xiaozhi/iot/resp/{mac}
    │
    └── Send_Announce()                # 发送设备声明（NULL安全）
        └── Publish → xiaozhi/iot/announce/{mac}
            └── device_id=1(一楼)/2(二楼)/3(三楼)
```

```
iot_control_task.c (设备控制层)
    │
    ├── Device_Refresh_Task()          # 10ms周期执行
    │   ├── 比较 g_device_flags 与 g_current_gpio_state
    │   ├── 调用 Set_Gpio_Level() 操作硬件
    │   └── 更新 g_current_servo_angle[] 驱动舵机
    │
    └── Iot_Control_Task_Init()        # 创建FreeRTOS任务
```

```
mqtt_heartbeat.c (心跳层)
    │
    └── Heartbeat_Task()               # 30s周期执行
        ├── MQTT_Get_Client() 获取Client
        ├── MQTT_Get_MAC_Str() 获取MAC
        ├── 构造 iot_heartbeat_packet_t（含MAC字符串）
        └── Publish → xiaozhi/iot/heartbeat/{mac}
```

### 3.4 从机初始化顺序

```
iot_slave_main.c → app_main()
    │
    ├── WiFi_Init()                    # 1. 先连WiFi
    ├── MQTT_Receive_Init()            # 2. 初始化MQTT（读取MAC、连接Broker）
    ├── Iot_Control_Task_Init()        # 3. 启动GPIO刷新任务
    └── MQTT_Heartbeat_Init()          # 4. 启动心跳任务（5s后首次发送）
```

> 初始化顺序保证：MQTT_Receive_Init先于Heartbeat_Init，确保s_mac_str和s_mqtt_client已初始化。

---

## 四、数据流详解

### 4.1 控制流程：主机开一楼灯光

```
用户语音"打开一楼客厅灯"
    ↓
小智AI解析Intent → MCP工具 set_light(device_id="1", gpio_index=0, state=1)
    ↓
App_IOT_Set_Light("一楼客厅灯", 0, true)
    ↓
IOT_Find_Device_By_Name("一楼客厅灯")
    → IOT_Resolve_Alias("一楼客厅灯") → "二楼设备"
    → 匹配 remote_devices[i].device_name == "二楼设备" → device_index
    ↓
IOT_Set_Light(dev, device_index, 0, 1)
    ↓
IOT_Set_Remote_GPIO(dev, device_index, 0, 1) → gpio_index=0
    ↓
构造命令包: {cmd=0x01, gpio_index=0, value=1}
    ↓
Topic: xiaozhi/iot/cmd/{device_id}
    → device_id 由 IOT_Get_Device_Id_By_Name("二楼设备") = 2
    → 发布到 "xiaozhi/iot/cmd/2"
    ↓
二楼从机收到 → Process_Command()
    → gpio_index=0 < LIGHT_COUNT(3) → g_device_flags.light[0] = ON
    ↓
发送响应 → MQTT Publish → "xiaozhi/iot/resp/{mac}"
    ↓
主机收到 → HW_MQTT_Recv_Callback()
    → 拷贝topic到栈缓冲区并null-terminate
    → 从Topic "xiaozhi/iot/resp/AABBCCDDEEFF" 解析MAC
    → IOT_Process_Remote_Response() → MAC匹配设备 → online=true
    ↓
iot_control_task 10ms后检测标志位变化 → 实际点亮GPIO
```

### 4.2 广播流程：关灯

```
用户语音"关闭所有灯"
    ↓
App_IOT_Broadcast_Lights_Off()
    ↓
IOT_Broadcast_Lights_Off(dev)
    ↓
构造命令包: {cmd=0x32}
    ↓
MQTT Publish → "xiaozhi/iot/cmd/broadcast"
    ↓
一楼/二楼/三楼同时收到 → Process_Command()
    → g_device_flags.light[0~2] = OFF (所有从机)
```

### 4.3 设备发现流程

```
主机启动 → App_IOT_Discover_Devices()
    ↓
MQTT Publish → "xiaozhi/iot/cmd/broadcast" {cmd=0x06 (DISCOVER)}
    ↓
所有从机收到 DISCOVER → Send_Announce()
    ↓
各从机发布到各自的Topic:
    一楼 → "xiaozhi/iot/announce/{mac1}" {cmd=0x08, device_id=1, name="一楼设备"}
    二楼 → "xiaozhi/iot/announce/{mac2}" {cmd=0x08, device_id=2, name="二楼设备"}
    三楼 → "xiaozhi/iot/announce/{mac3}" {cmd=0x08, device_id=3, name="三楼设备"}
    ↓
主机收到 → HW_MQTT_Recv_Callback()
    → 从Topic "xiaozhi/iot/announce/AABBCCDDEEFF" 解析MAC
    → IOT_Process_Remote_Response(mac, data, len)
    ├── MAC匹配已有设备 → 更新online=true + 设备信息
    └── 新设备 → IOT_Add_Remote_Device()
        → device_id = IOT_Get_Device_Id_By_Name("一楼设备") = 1
        → 后续命令路由到 "xiaozhi/iot/cmd/1" ✓
```

> **关键**：Announce Topic包含MAC，主机能正确区分不同设备。device_id由设备名推导，不依赖发现顺序。

### 4.4 断线重连流程

```
MQTT连接断开
    ↓
ESP-IDF MQTT客户端自动重连
    ↓
重连成功 → MQTT_EVENT_CONNECTED 事件
    ↓
主机 mqtt_event_handler() 处理:
    ├── 重新订阅 xiaozhi/iot/resp/+
    ├── 重新订阅 xiaozhi/iot/heartbeat/+
    └── 重新订阅 xiaozhi/iot/announce/+
    ↓
从机 mqtt_event_handler() 处理:
    ├── 重新订阅 xiaozhi/iot/cmd/broadcast
    ├── 重新订阅 xiaozhi/iot/cmd/{device_id}
    └── 重新发送 Send_Announce()（主机可重新发现设备）
```

---

## 五、设备GPIO映射表

### 5.1 GPIO索引统一映射

| 设备类型 | GPIO索引范围 | 一楼 | 二楼 | 三楼 |
|----------|-------------|------|------|------|
| 灯光 | 0~2 | 浴室灯/客厅灯/卧室灯 | 浴室灯/客厅灯/卧室灯 | 卧室灯/客厅灯/西庭灯 |
| 继电器 | 3~5 | 电视/空调/热水器 | 电视/空调/热水器 | 电视/空调/热水器 |
| 舵机 | 6~8 | 窗户/窗帘/门锁 | 晾衣架/窗帘/门锁 | 左天窗/右天窗/晾衣架 |

### 5.2 舵机角度映射

| 楼层 | servo[0,1] 有效角度 | servo[2] 有效角度 | 枚举映射 |
|------|---------------------|-------------------|----------|
| 一楼 | 0°, 45°, 90°, 135°, 180° | 0°, 45°, 90°, 135°, 180° | value/45 → SERVO_0~SERVO_180 |
| 二楼 | 0°, 45°, 90°, 135°, 180° | 0°, 45°, 90°, 135°, 180° | value/45 → SERVO_0~SERVO_180 |
| 三楼 | 0°, 25°, 70°, 135° | 0°, 180° | 条件映射 → SERVO_0~SERVO_180 |

> 三楼servo角度无效时不发送响应，主机不会误认为操作成功。

### 5.3 三楼额外传感器（本地处理，不上报MQTT）

| 传感器 | 引脚 | 处理方式 |
|--------|------|----------|
| 烟雾传感器 | ADC_CHANNEL_2 | 本地ADC轮询 → ASR UART报警 |
| 雨滴传感器 | ADC_CHANNEL_0 | 本地ADC轮询 → 自动控制舵机 |
| 求助按钮 | GPIO4 | 中断检测 → ASR UART报警 |

---

## 六、关键配置项

### 6.1 MQTT Broker

| 配置项 | 默认值 | 说明 |
|--------|--------|------|
| Broker URI | `mqtt://broker.emqx.io` | 公共测试Broker |
| 端口 | `1883` | 未加密TCP端口 |
| 主机Client ID | `xiaozhi_master` | 小智主机标识 |
| 从机1 Client ID | `xiaozhi_slave_1` | 一楼 |
| 从机2 Client ID | `xiaozhi_slave_2` | 二楼 |
| 从机3 Client ID | `xiaozhi_slave_3` | 三楼 |

### 6.2 超时与心跳

| 参数 | 值 | 说明 |
|------|----|------|
| 心跳间隔 | 30秒 | 从机定时发送 |
| 设备超时 | 60秒 | 主机检测掉线 |
| 舵机刷新 | 100ms | 舵机角度更新间隔 |
| GPIO刷新 | 10ms | GPIO状态检测间隔 |

### 6.3 关键常量

| 常量 | 值 | 说明 |
|------|----|------|
| `IOT_MAX_REMOTE_DEVICES` | 4 | 最多4个远程设备 |
| `IOT_MAX_REMOTE_GPIOS` | 16 | 每设备最多16个GPIO（支持servo[2]的gpio_index=8） |
| `IOT_DEVICE_NAME_LEN` | 32 | 设备名称最大长度 |

---

## 七、设备名别名系统

主机通过 `IOT_Resolve_Alias()` 将用户自然语言设备名映射到系统设备名：

| 用户说法 | 解析结果 | 目标设备 |
|----------|----------|----------|
| 一楼、一楼大门、大厅灯 | 一楼设备 | device_id=1 |
| 二楼、主卧、主卧灯、次卧、客厅、风扇、次卧灯、客厅灯 | 二楼设备 | device_id=2 |
| 三楼、阳台灯 | 三楼设备 | device_id=3 |

> device_id由 `IOT_Get_Device_Id_By_Name()` 根据设备名中的楼层关键词推导，确保命令路由正确。

---

## 八、安全机制

### 8.1 Topic字符串安全

ESP-MQTT事件回调中的 `topic` 指针**不是null-terminated**的。主机在 `HW_MQTT_Recv_Callback()` 中：
1. 将topic拷贝到128字节栈缓冲区
2. 手动添加null终止符
3. 在null-terminated缓冲区上执行 `strstr()`/`strlen()`

### 8.2 NULL安全

- 从机 `Send_Response()`/`Send_Announce()` 在调用前检查 `s_mqtt_client != NULL`
- 主机 `HW_MQTT_Publish()` 在调用前检查 `s_mqtt_client != NULL`
- `MQTT_Get_MAC_Str()` 返回静态数组指针，始终非NULL（可能为空字符串）

### 8.3 边界保护

- `IOT_Process_Remote_Response()` 中 `gpio_count` 限制不超过 `IOT_MAX_REMOTE_GPIOS`
- `IOT_Set_Servo()` 中 `gpio_index = 6 + servo_index` 检查不超过 `gpio_count`
- `App_IOT_Scenario_Homecoming()` 中检查 `device_index >= 0` 再操作

---

## 九、与ESP-NOW架构的关键差异

| 方面 | ESP-NOW (旧) | MQTT (新) |
|------|-------------|-----------|
| 网络依赖 | 无需路由器 | 需要WiFi路由器+MQTT Broker |
| 延迟 | 极低（直接广播） | 稍高（需经过Broker） |
| 距离 | 局域网直连 | 只要有网络即可（可远程） |
| 设备发现 | 依赖MAC预配 | 自动ANNOUNCE（Topic含MAC） |
| 设备标识 | MAC地址（自动获取） | Topic解析MAC + device_id |
| device_id路由 | MAC直连无需路由 | Topic路由: cmd/{device_id} |
| 多主机支持 | 困难 | 天然支持（订阅同一Topic） |
| 状态同步 | 无 | 可通过响应Topic同步 |
| P4面板 | 不支持ESP-NOW | 支持（同一MQTT网络） |
| 断线重连 | 无（ESP-NOW无连接） | 自动重连+重新订阅 |
| 心跳识别 | MAC自动获取 | 从Topic解析MAC |
| Topic安全 | N/A | 需null-terminate后才能用字符串函数 |

---

## 十、已知限制

| 限制 | 说明 | 影响 |
|------|------|------|
| 三楼传感器无法通过MCP查询 | 传感器数据仅在从机本地处理 | 小智无法主动查询烟雾/雨滴值 |
| 药盒从机未集成 | `xiaozhi_slave_yaohe` 未纳入MQTT网络 | 药盒MCP工具无法使用 |
| 公共Broker不稳定 | `broker.emqx.io` 为测试用 | 生产环境需自建Broker |
| WiFi依赖 | 所有设备必须连接同一WiFi | WiFi断网则IoT控制失效 |
| P4面板未集成 | lvgl_prj尚未接入MQTT | P4无法控制从机设备 |

---

## 十一、Bug修复历史

### v3.0 修复（2026-04-20）

| 级别 | Bug | 修复方案 |
|------|-----|----------|
| CRITICAL | MQTT topic非null-terminated，`strstr()`/`strlen()`是UB | 拷贝topic到栈缓冲区并null-terminate |
| CRITICAL | Announce Topic无MAC，多设备MAC={0}碰撞 | 改为 `announce/{mac}` Topic，主机订阅 `announce/+` |
| HIGH | device_id基于发现顺序，命令可能路由到错误设备 | 从机设置正确device_id(1/2/3)，主机用 `IOT_Get_Device_Id_By_Name()` 推导 |
| HIGH | 别名表"次卧灯"→"客厅灯"→"厕所灯"映射到不存在的设备名 | 修正为"二楼设备" |
| MEDIUM | 三楼servo角度无效仍发响应，主机误以为成功 | 仅在角度有效时发响应，无效时打警告日志 |
| MEDIUM | `Send_Response`/`Send_Announce`未检查client NULL | 添加 `if (s_mqtt_client == NULL) return` |
| MEDIUM | `IOT_Find_GPIO_By_Name`声明未实现 | 移除未实现的声明，避免链接错误 |
| LOW | 注释仍引用 `esp_now_receive.c` | 更新为 `mqtt_receive.c` |
| LOW | 二楼 `iot_control_task.h` 标题写"一楼设备" | 修正为"二楼设备" |

### v2.0 修复（2026-04-19）

| 级别 | Bug | 修复方案 |
|------|-----|----------|
| CRITICAL | `g_iot_controller` 重复定义 | 移除前向声明，保留初始化定义 |
| CRITICAL | `HW_MQTT_Init` 事件回调为NULL | 创建 `mqtt_event_handler` 并注册 |
| HIGH | `mqtt_heartbeat.c` extern跨文件引用 | 改用 `MQTT_Get_Client()`/`MQTT_Get_MAC_Str()` getter |
| HIGH | 主机心跳无法区分设备（MAC全零） | 从Topic解析MAC |
| HIGH | `IOT_MAX_REMOTE_GPIOS=8` 导致servo[2]不可控 | 改为16 |
| HIGH | 一楼设备名显示"二楼"（复制粘贴错误） | 修正所有"二楼"为"一楼" |
| MEDIUM | MQTT重连后未重新订阅 | 移到MQTT_EVENT_CONNECTED处理 |
| MEDIUM | Homecoming场景device_index可能为-1 | 添加 `>= 0` 检查 |
| MEDIUM | 状态查询只缩短"二楼设备" | 添加所有三层楼名称缩短 |

---

*文档版本: v3.0*
*更新日期: 2026-04-20*
*适用范围: ESP32-S3主机 + 一楼/二楼/三楼从机 + P4面板(预留)*
