# 设备控制模块优化与修复实施计划

## 项目概述

针对小智智慧庭院系统的设备控制模块，进行5大方面的优化与修复工作。项目采用主机(ESP32-S3) + 从机(ESP32) 架构，通过 MQTT 通信控制庭院设备。

---

## 任务1：传感器架构调整 — 光照传感器与雨滴传感器物理及逻辑隔离

### 问题分析

当前系统存在严重的传感器耦合问题：
- **物理层**：雨滴检测复用光照传感器 ADC 通道（`LIGHT_ADC_CHANNEL = ADC1_CHANNEL_2`），`sensor_config.h` 中 `RAIN_THRESHOLD_LIGHT` 使用光照阈值判断雨天
- **逻辑层**：`sensor_adc_task.c` 中 `rain_wet` 变量直接由 `light_mv` 值推导，夜间光照自然低于阈值必然误报
- **命名层**：`IOT_SENSOR_SMOKE_MV` / `IOT_SENSOR_RAIN_MV` 枚举名与实际含义不匹配（实际是土壤/光照）

### 实施步骤

#### 1.1 从机硬件配置层 — 新增独立雨滴传感器 ADC 通道

**文件**: `slave/gardenslave/main/APP/Inc/sensor_config.h`

- 新增 `RAIN_ADC_CHANNEL` 定义（如 `ADC1_CHANNEL_3`，对应 GPIO4 或其他可用引脚）
- 新增 `RAIN_ADC_ATTEN` 定义
- 保留 `LIGHT_ADC_CHANNEL` 仅用于光照检测
- 新增 `RAIN_THRESHOLD_MV` 替代 `RAIN_THRESHOLD_LIGHT`，阈值基于雨滴传感器特性设定
- 删除 `RAIN_THRESHOLD_LIGHT` 宏

#### 1.2 从机传感器采集层 — 分离采集与处理逻辑

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

- `HW_ADC_Init()` 中新增 `adc1_config_channel_atten(RAIN_ADC_CHANNEL, RAIN_ADC_ATTEN)`
- `Sensor_ADC_Task()` 中：
  - `light_mv` 仅从 `LIGHT_ADC_CHANNEL` 读取，用于光照强度计算
  - 新增 `rain_mv` 从 `RAIN_ADC_CHANNEL` 读取，用于雨滴检测
  - `rain_wet` 改为基于 `rain_mv` 判断（`rain_mv <= RAIN_THRESHOLD_MV`）
  - MQTT 上报 JSON 中新增 `"rain_mv"` 字段，保留 `"light_mv"` 字段
- 日志输出分离光照和雨滴数据

#### 1.3 共享协议层 — 修正传感器类型枚举命名

**文件**: `shared/mqtt_iot_protocol.h`

- `IOT_SENSOR_SMOKE_MV` → `IOT_SENSOR_SOIL_MV`
- `IOT_SENSOR_RAIN_MV` → `IOT_SENSOR_LIGHT_MV`
- 新增 `IOT_SENSOR_RAIN_ADC_MV = 5` 用于独立雨滴传感器 ADC 值
- 添加枚举值注释说明

#### 1.4 主机 BSP 层 — 修正传感器字段映射

**文件**: `main/BSP/Inc/bsp_iot_controller.h`

- `iot_sensor_state_t` 结构体中：
  - `smoke_mv` → `soil_mv`
  - `rain_mv` → `light_mv`
  - 新增 `rain_adc_mv` 字段

**文件**: `main/BSP/Src/bsp_iot_controller.c`

- `IOT_Process_Sensor_Data()` 中 JSON 解析字段名同步更新
- `IOT_Get_Sensors_Json()` 输出字段名同步更新

#### 1.5 主机 APP 层 — 修正传感器数据映射

**文件**: `main/BSP/Inc/bsp_garden.h`

- `garden_sensor_data_t` 中 `soil_value` → `soil_percent`，`light_value` → `light_percent`
- 新增 `rain_adc_mv` 字段

**文件**: `main/APP/Src/app_garden.c`

- `Garden_Sensor_Callback()` 中二进制协议映射修正：
  - `IOT_SENSOR_SOIL_MV` → `soil_percent`
  - `IOT_SENSOR_LIGHT_MV` → `light_percent`
  - 新增 `IOT_SENSOR_RAIN_ADC_MV` → `rain_adc_mv`
- JSON 解析同步更新字段名

#### 1.6 LVGL 仿真层 — 同步更新

**文件**: `lvgl_sim/main/UI/model/garden_model.c` / `garden_model.h`

- 传感器数据模型字段名同步更新

---

## 任务2：功能模块清理 — 彻底移除贪睡功能

### 问题分析

贪睡(Snooze)功能涉及以下代码：
- `alarm_mcp_tool.h` 中 `ALARM_STATE_SNOOZED` 枚举、`snooze_count`/`snooze_minutes` 字段、`Alarm_Mcp_Snooze()` 函数声明
- `alarm_mcp_tool.cpp` 中 `Alarm_Mcp_Snooze()` 实现、`Add_Alarm_Internal()` 中 snooze 初始化、`Alarm_Mcp_Get_Status_Json()` 中 snooze 字段输出、`Alarm_Mcp_Process()` 中缺少 SNOOZED 状态处理（S7 bug）
- MCP 工具注册中 `self.alarm.snooze` 工具（需检查 `custom_esp32s3_cam.cc`）

### 实施步骤

#### 2.1 移除贪睡枚举与结构体字段

**文件**: `main/boards/custom-esp32s3-cam/alarm_mcp_tool.h`

- 删除 `ALARM_STATE_SNOOZED = 3` 枚举值
- 删除 `alarm_info_t` 中 `snooze_count` 和 `snooze_minutes` 字段
- 删除 `Alarm_Mcp_Snooze()` 函数声明

#### 2.2 移除贪睡实现代码

**文件**: `main/boards/custom-esp32s3-cam/alarm_mcp_tool.cpp`

- 删除 `Alarm_Mcp_Snooze()` 函数实现
- `Add_Alarm_Internal()` 中删除 `alarm->snooze_count = 0` 和 `alarm->snooze_minutes = 5` 初始化
- `Alarm_Mcp_Get_Status_Json()` 中删除 `snooze_count` 字段输出
- `Alarm_Mcp_Process()` 中无需再处理 SNOOZED 状态（已移除该枚举）

#### 2.3 移除贪睡 MCP 工具注册

**文件**: `main/boards/custom-esp32s3-cam/custom_esp32s3_cam.cc`

- 搜索并移除 `self.alarm.snooze` 工具注册代码（如果存在）
- 检查 `Register_Alarm_Mcp_Tools()` 中的贪睡相关工具

#### 2.4 清理 NVS 兼容性

- `Load_Alarms_From_NVS()` 中保留对旧 NVS 数据的容错处理（忽略 snooze 字段即可，因为结构体已缩减，旧数据加载时 `nvs_get_blob` 会返回长度不匹配错误，需处理该情况）

---

## 任务3：传感器映射修复

### 问题分析

当前映射关系混乱：
- 协议枚举 `IOT_SENSOR_SMOKE_MV` 实际对应土壤传感器，`IOT_SENSOR_RAIN_MV` 实际对应光照传感器
- BSP 层 `iot_sensor_state_t` 中 `smoke_mv`/`rain_mv` 字段名与含义不匹配
- APP 层 `garden_sensor_data_t` 中 `soil_value`/`light_value` 存储的是百分比值而非原始 mV 值
- `app_garden.c` 二进制协议解析中 `IOT_SENSOR_SMOKE_MV` → `soil_value`，`IOT_SENSOR_RAIN_MV` → `light_value`，映射链路错误

### 实施步骤

#### 3.1 建立正确的传感器映射关系表

| 物理传感器 | ADC通道 | 协议枚举 | BSP字段 | APP字段 | MQTT JSON字段 |
|---|---|---|---|---|---|
| 土壤湿度 | ADC1_CH0 | `IOT_SENSOR_SOIL_MV` | `soil_mv` | `soil_percent` | `soil_mv` |
| 光照强度 | ADC1_CH2 | `IOT_SENSOR_LIGHT_MV` | `light_mv` | `light_percent` | `light_mv` |
| 雨滴检测 | ADC1_CH3(新增) | `IOT_SENSOR_RAIN_ADC_MV` | `rain_adc_mv` | `rain_adc_mv` | `rain_mv` |
| 火焰状态 | GPIO10 | `IOT_SENSOR_FIRE_STATUS` | `fire_status` | `fire_status` | `fire_status` |
| 雨天状态 | 由rain_adc推导 | `IOT_SENSOR_RAIN_STATUS` | `rain_status` | `rain_state` | `rain_status` |
| 求助状态 | GPIO11 | `IOT_SENSOR_HELP_STATUS` | `help_status` | `help_status` | `help_status` |

#### 3.2 修正协议枚举

**文件**: `shared/mqtt_iot_protocol.h`

（已在任务1.3中覆盖）

#### 3.3 修正 BSP 层映射

**文件**: `main/BSP/Inc/bsp_iot_controller.h` + `main/BSP/Src/bsp_iot_controller.c`

（已在任务1.4中覆盖）

#### 3.4 修正 APP 层映射

**文件**: `main/BSP/Inc/bsp_garden.h` + `main/APP/Src/app_garden.c`

（已在任务1.5中覆盖）

#### 3.5 修正从机上报字段

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

- MQTT JSON 上报字段名确认：`soil_mv`、`light_mv`、`rain_mv`（新增）、`fire_status`、`rain_status`、`help_status`、`temperature`、`humidity`

---

## 任务4：命名规范优化

### 问题分析

当前命名存在以下问题：
- 传感器枚举名与实际含义不匹配（`SMOKE_MV` 实为土壤、`RAIN_MV` 实为光照）
- BSP 层字段名 `smoke_mv`/`rain_mv` 含义错误
- APP 层 `soil_value`/`light_value` 存的是百分比却叫 value
- `garden_light_state_t` 被复用于风扇、水泵等非灯光设备
- `App_Garden_Set_Servo` 中 gpio_index 硬编码为 6（魔术数字）
- 中文设备名宏 `LIGHT_1_CHINESE` 等使用数字编号缺乏业务含义
- `device_onoff_enum_t` 命名不够专业

### 实施步骤

#### 4.1 传感器命名修正（已在任务1/3中覆盖）

#### 4.2 设备状态类型命名优化

**文件**: `main/BSP/Inc/bsp_garden.h`

- `garden_light_state_t` → `garden_switch_state_t`（通用开关状态，适用于灯/风扇/水泵等）
- `GARDEN_LIGHT_OFF` → `GARDEN_SWITCH_OFF`
- `GARDEN_LIGHT_ON` → `GARDEN_SWITCH_ON`
- `garden_sensor_data_t.soil_value` → `soil_percent`
- `garden_sensor_data_t.light_value` → `light_percent`

**文件**: `slave/gardenslave/main/APP/Inc/iot_control_task.h`

- `device_onoff_enum_t` → `device_switch_state_t`
- `OFF` → `SWITCH_OFF`
- `ON` → `SWITCH_ON`
- `servo_angle_limits_t` → `servo_angle_limit_t`（单数形式更准确）

#### 4.3 消除魔术数字

**文件**: `main/APP/Src/app_garden.c`

- `App_Garden_Set_Servo()` 中 `gpio_index = 6` → 定义 `#define SERVO_GPIO_INDEX_BASE 6` 或使用枚举

**文件**: `main/BSP/Src/bsp_iot_controller.c`

- `IOT_Set_Relay()` 中 `gpio_index = 3 + relay_index` → 定义 `#define RELAY_GPIO_INDEX_BASE 3`
- `IOT_Set_Servo()` 中 `gpio_index = 6 + servo_index` → 使用 `SERVO_GPIO_INDEX_BASE`

#### 4.4 中文设备名宏优化

**文件**: `slave/gardenslave/main/APP/Inc/iot_control_task.h`

- `LIGHT_1_CHINESE` → `LIGHT_COURTYARD_WEST_NAME`
- `LIGHT_2_CHINESE` → `LIGHT_STRIP_NAME`
- `LIGHT_3_CHINESE` → `LIGHT_CORRIDOR_LONG_NAME`
- `RELAY_1_CHINESE` → `RELAY_FAN_NAME`
- `RELAY_2_CHINESE` → `RELAY_WATER_PUMP_NAME`
- `RELAY_3_CHINESE` → `RELAY_ATOMIZER_NAME`
- `RELAY_4_CHINESE` → `RELAY_SAFETY_SWITCH_NAME`
- `SERVO_1_CHINESE` → `SERVO_CLOTHES_NAME`

#### 4.5 命名变更对照表

| 旧命名 | 新命名 | 影响文件 |
|---|---|---|
| `IOT_SENSOR_SMOKE_MV` | `IOT_SENSOR_SOIL_MV` | mqtt_iot_protocol.h, bsp_iot_controller.c, app_garden.c |
| `IOT_SENSOR_RAIN_MV` | `IOT_SENSOR_LIGHT_MV` | mqtt_iot_protocol.h, bsp_iot_controller.c, app_garden.c |
| `iot_sensor_state_t.smoke_mv` | `soil_mv` | bsp_iot_controller.h, bsp_iot_controller.c |
| `iot_sensor_state_t.rain_mv` | `light_mv` | bsp_iot_controller.h, bsp_iot_controller.c |
| `garden_light_state_t` | `garden_switch_state_t` | bsp_garden.h, app_garden.h, app_garden.c, garden_mcp_tool.cpp |
| `GARDEN_LIGHT_OFF/ON` | `GARDEN_SWITCH_OFF/ON` | 同上 |
| `device_onoff_enum_t` | `device_switch_state_t` | iot_control_task.h, iot_control_task.c, sensor_adc_task.c, mqtt_receive.c |
| `OFF/ON` | `SWITCH_OFF/SWITCH_ON` | 同上 |
| `soil_value` | `soil_percent` | bsp_garden.h, app_garden.c |
| `light_value` | `light_percent` | bsp_garden.h, app_garden.c |
| `LIGHT_1_CHINESE` | `LIGHT_COURTYARD_WEST_NAME` | iot_control_task.h |
| `RAIN_THRESHOLD_LIGHT` | `RAIN_THRESHOLD_MV` | sensor_config.h, sensor_adc_task.c |

---

## 任务5：代码质量约束优化

### 5.1 修复原子操作竞争条件（S1）

**文件**: `slave/gardenslave/main/APP/Src/iot_control_task.c`

- 删除 `atomic_flag_exchange()` 函数
- 将 `g_fire_detected`/`g_help_detected`/`g_rain_alert_triggered` 改用 `xTaskNotifyFromISR` / `xTaskNotifyWait` 机制
- ISR 中使用 `xTaskNotifyFromISR(iot_ctrl_task_handle, FIRE_NOTIFY_BIT, eSetBits, ...)`
- `Device_Refresh_Task` 中使用 `xTaskNotifyWait` 等待通知

### 5.2 移除 volatile 与互斥锁混用（S2）

**文件**: `slave/gardenslave/main/APP/Inc/iot_control_task.h` + `Src/iot_control_task.c`

- `g_device_flags` 保留 `volatile`（因为被 ISR 和多任务访问），但确保所有访问都在 `DEVICE_FLAGS_LOCK/UNLOCK` 保护下
- `g_rain_alert_triggered`/`g_help_detected` 改用 TaskNotify 后不再需要 `volatile`

### 5.3 修复本地 GPIO 声明无实现（S3/S4）

**文件**: `main/BSP/Inc/bsp_iot_controller.h`

- 移除 `IOT_Add_Local_GPIO()` 函数声明

**文件**: `main/APP/Src/iot_controller.c`

- 移除 `LOCAL_GPIO_1_PIN` ~ `LOCAL_GPIO_4_PIN` 定义
- 移除 `HW_Gpio_Init`/`HW_Gpio_Write`/`HW_Gpio_Read` 函数（如果仅用于本地 GPIO）
- 或实现 `IOT_Add_Local_GPIO()` 并在 `App_IOT_System_Init()` 中注册本地 GPIO

### 5.4 修复广播开灯不更新状态缓存（M1）

**文件**: `main/BSP/Src/bsp_iot_controller.c`

- `IOT_Broadcast_Lights_On()` 发布成功后更新 `gpio_states[0..2] = 1`，与 `IOT_Broadcast_Lights_Off()` 行为对称

### 5.5 修复火灾双重广播（M2）

**文件**: `main/APP/Src/app_garden.c`

- `App_Garden_Fire_Emergency_Response()` 中删除直接 publish 的 `IOT_CMD_EMERGENCY` 命令
- 仅保留 `App_IOT_Scenario_Emergency()` 调用（该函数内部已发送紧急广播）

### 5.6 修复心跳间隔等于 keepalive（M11）

**文件**: `slave/gardenslave/main/APP/Src/mqtt_heartbeat.c`

- `HEARTBEAT_INTERVAL_MS` 从 30000 改为 10000（keepalive 30s 的 1/3）

### 5.7 修复雨滴计数溢出（M12）

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

- `g_rain_exceed_count` 改为 `uint16_t` 并添加上限检查：`if (g_rain_exceed_count < RAIN_TRIGGER_COUNT + 10) g_rain_exceed_count++`

### 5.8 修复自动控制与远程控制冲突（M13）

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

- 自动模式控制逻辑中，添加"远程控制冷却期"：当设备被远程指令控制后，自动模式在 N 秒内不覆盖该设备状态
- 或采用"自动模式只建议不强制"策略：自动模式仅设置标志位，由用户确认后执行

### 5.9 清理死代码

- `garden_mcp_tool.cpp` 中删除 `Garden_Mcp_Start()` 空函数体和 `Garden_Mcp_Init()` 无实质逻辑函数
- `garden_mcp_tool.h` 中删除对应声明
- `custom_esp32s3_cam.cc` 中删除 `g_i2c_mutex` 声明
- `bsp_garden.c` 补充实际 BSP 抽象实现或删除空文件
- `app_asr_uart.c` 中删除 `App_ASR_Send_Emergency_Alert()` 死代码

### 5.10 ADC 参考电压校准（L13）

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

- 使用 `esp_adc_cal` 组件进行 ADC 特性校准，替代硬编码 3100mV 参考电压

### 5.11 MQTT 数据包对齐安全（L14）

**文件**: `slave/gardenslave/main/APP/Src/mqtt_receive.c`

- `Process_Command()` 中对 `iot_command_packet_t*` 强转前，先 `memcpy` 到局部变量避免对齐问题

### 5.12 协议命令码注释（L16）

**文件**: `shared/mqtt_iot_protocol.h`

- 添加命令码分区注释：0x01-0x08 基础命令、0x10-0x13 设备控制、0x30-0x38 广播与场景

---

## 实施顺序与依赖关系

```
阶段1（基础修正，无依赖）:
  ├── 5.6 心跳间隔修复
  ├── 5.7 雨滴计数溢出修复
  ├── 5.9 死代码清理
  ├── 5.12 协议注释
  └── 2.x 贪睡功能移除

阶段2（传感器隔离，核心变更）:
  ├── 1.1 硬件配置层新增雨滴通道
  ├── 1.2 采集层分离
  ├── 1.3 协议枚举修正
  ├── 1.4 BSP层映射修正
  ├── 1.5 APP层映射修正
  └── 1.6 LVGL仿真层同步

阶段3（命名规范，依赖阶段2）:
  ├── 4.2 设备状态类型命名
  ├── 4.3 消除魔术数字
  ├── 4.4 中文设备名宏优化
  └── 4.5 命名变更对照表验证

阶段4（代码质量，部分依赖阶段2/3）:
  ├── 5.1 原子操作修复
  ├── 5.2 volatile清理
  ├── 5.3 本地GPIO修复
  ├── 5.4 状态缓存修复
  ├── 5.5 双重广播修复
  ├── 5.8 自动控制冲突修复
  ├── 5.10 ADC校准
  └── 5.11 数据包对齐安全

阶段5（验证与文档）:
  ├── 编译验证
  ├── 传感器映射关系文档
  └── 命名变更对照表
```

---

## 涉及文件清单

### 从机文件（slave/gardenslave/main/）
| 文件 | 变更类型 |
|---|---|
| `APP/Inc/sensor_config.h` | 修改：新增雨滴ADC通道定义 |
| `APP/Inc/sensor_adc_task.h` | 不变 |
| `APP/Inc/iot_control_task.h` | 修改：枚举重命名、宏重命名 |
| `APP/Inc/mqtt_receive.h` | 不变 |
| `APP/Src/sensor_adc_task.c` | 修改：分离光照/雨滴采集、修复溢出、ADC校准 |
| `APP/Src/iot_control_task.c` | 修改：原子操作改TaskNotify、volatile清理 |
| `APP/Src/mqtt_receive.c` | 修改：数据包对齐安全 |
| `APP/Src/mqtt_heartbeat.c` | 修改：心跳间隔 |
| `APP/Src/app_asr_uart.c` | 修改：删除死代码 |

### 主机文件（main/）
| 文件 | 变更类型 |
|---|---|
| `BSP/Inc/bsp_iot_controller.h` | 修改：字段重命名、删除IOT_Add_Local_GPIO声明 |
| `BSP/Inc/bsp_garden.h` | 修改：类型重命名、字段重命名 |
| `BSP/Src/bsp_iot_controller.c` | 修改：字段映射修正、状态缓存修复 |
| `BSP/Src/bsp_garden.c` | 修改：补充BSP实现或删除 |
| `APP/Inc/app_garden.h` | 修改：函数签名类型更新 |
| `APP/Src/app_garden.c` | 修改：映射修正、双重广播修复、魔术数字消除 |
| `APP/Src/iot_controller.c` | 修改：本地GPIO清理 |
| `boards/custom-esp32s3-cam/alarm_mcp_tool.h` | 修改：删除贪睡相关 |
| `boards/custom-esp32s3-cam/alarm_mcp_tool.cpp` | 修改：删除贪睡实现 |
| `boards/custom-esp32s3-cam/garden_mcp_tool.h` | 修改：删除死代码声明 |
| `boards/custom-esp32s3-cam/garden_mcp_tool.cpp` | 修改：删除死代码、类型更新 |
| `boards/custom-esp32s3-cam/custom_esp32s3_cam.cc` | 修改：删除g_i2c_mutex、贪睡MCP工具 |

### 共享文件
| 文件 | 变更类型 |
|---|---|
| `shared/mqtt_iot_protocol.h` | 修改：枚举重命名、新增枚举值、注释 |

### LVGL仿真文件
| 文件 | 变更类型 |
|---|---|
| `lvgl_sim/main/UI/model/garden_model.c` | 修改：字段名同步 |
| `lvgl_sim/main/UI/model/garden_model.h` | 修改：字段名同步 |
