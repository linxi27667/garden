# 阶段5：编译验证与最终检查 — 实施计划

## 当前状态

阶段1-4的所有代码修改已完成并验证通过（15/15文件确认变更到位）。本计划覆盖阶段5的剩余工作。

---

## 5.1 编译验证

### 5.1.1 从机工程编译

```bash
cd e:\MCU\esp32\xaiozhi1111111111111111111111111\garden\slave\gardenslave
idf.py build
```

**预期问题**：
- `sensor_adc_task.c` 中 `RAIN_ADC_CHANNEL` 对应的 GPIO 引脚可能需要在 `sdkconfig` 或板级配置中确认
- `xTaskNotify` 相关的 FreeRTOS API 需确认 ESP-IDF 版本支持
- `Sensor_ADC_Notify_Relay_Manual()` 新增函数的调用者需全部链接通过

**修复策略**：根据编译错误逐一修复，优先确保零 error，warning 逐条评估

### 5.1.2 主机工程编译

```bash
cd e:\MCU\esp32\xaiozhi1111111111111111111111111\garden
idf.py build
```

**预期问题**：
- `bsp_garden.c` 空文件可能导致链接器未找到某些预期符号
- `garden_switch_state_t` / `GARDEN_SWITCH_OFF/ON` 重命名需确保所有引用文件已同步
- `alarm_mcp_tool.cpp` 中 NVS blob 大小变更可能导致加载旧数据时的兼容性处理

---

## 5.2 遗留代码问题修复

### 5.2.1 bsp_garden.c 空文件处理

**文件**: `main/BSP/Src/bsp_garden.c`

当前仅 1 行 `#include "bsp_garden.h"`，BSP 层庭院硬件抽象完全缺失。

**方案**：检查 `bsp_garden.h` 中声明的函数，在 `bsp_garden.c` 中实现基础桩函数（返回默认值），确保编译链接通过。具体实现取决于 `bsp_garden.h` 的声明。

### 5.2.2 iot_controller.c 本地 GPIO 清理

**文件**: `main/APP/Src/iot_controller.c`

当前状态：
- `LOCAL_GPIO_1_PIN` ~ `LOCAL_GPIO_4_PIN` 定义仍存在（L34-37）
- `HW_Gpio_Init/Write/Read` 函数仍存在（L39-60），但它们被注册为函数指针（L263-265），**不是死代码**
- `local_gpio_count` 始终为 0（L261），本地 GPIO 功能不可用

**方案**：
- 保留 `HW_Gpio_Init/Write/Read`（它们是框架必要的硬件绑定层）
- 移除 `LOCAL_GPIO_1_PIN` ~ `LOCAL_GPIO_4_PIN` 定义（从未被使用）
- 在 `bsp_iot_controller.h` 中 `IOT_Add_Local_GPIO()` 声明已移除（已确认）

### 5.2.3 LVGL 仿真层命名一致性

**文件**: `lvgl_sim/main/UI/model/garden_model.h`

当前字段命名与主工程不一致：
- `smoke_sensor` → 应改为 `soil_sensor`（与 `soil_percent`/`soil_mv` 一致）
- `rain_sensor` → 应改为 `rain_adc_raw`（与 `rain_adc_mv` 区分）

**文件**: `lvgl_sim/main/UI/model/garden_model.c`

同步更新字段引用。

### 5.2.4 ADC 参考电压校准（5.10 — 可选）

**文件**: `slave/gardenslave/main/APP/Src/sensor_adc_task.c`

当前硬编码 3100mV 参考电压。使用 `esp_adc_cal` 组件可提高精度，但：
- 需确认 ESP-IDF 版本是否包含 `esp_adc_cal` 组件
- 需确认目标芯片是否支持 eFuse 校准数据
- 此项为优化项，不影响功能正确性

**方案**：标记为后续优化项，当前阶段不实施

---

## 5.3 传感器映射关系文档

整理完整的传感器映射链路表，从物理传感器到 MQTT JSON 字段的全链路映射：

| 物理传感器 | ADC通道/GPIO | 协议枚举 | BSP字段 | APP字段 | MQTT JSON字段 | 单位 |
|---|---|---|---|---|---|---|
| 土壤湿度 | ADC1_CH0 | `IOT_SENSOR_SOIL_MV=0` | `soil_mv` | `soil_percent` | `soil_mv` | mV→% |
| 光照强度 | ADC1_CH2 | `IOT_SENSOR_LIGHT_MV=1` | `light_mv` | `light_percent` | `light_mv` | mV→% |
| 雨滴检测 | ADC1_CH3(新增) | `IOT_SENSOR_RAIN_ADC_MV=5` | `rain_adc_mv` | `rain_adc_mv` | `rain_mv` | mV |
| 火焰状态 | GPIO10 | `IOT_SENSOR_FIRE_STATUS=2` | `fire_status` | `fire_status` | `fire_status` | 0/1/2 |
| 雨天状态 | 由rain_adc推导 | `IOT_SENSOR_RAIN_STATUS=3` | `rain_status` | `rain_state` | `rain_status` | 0/1 |
| 求助状态 | GPIO11 | `IOT_SENSOR_HELP_STATUS=4` | `help_status` | `help_status` | `help_status` | 0/1 |
| 温度 | DHT11/GPIO8 | JSON直传 | - | `temperature` | `temperature` | °C |
| 湿度 | DHT11/GPIO8 | JSON直传 | - | `humidity` | `humidity` | % |

---

## 5.4 命名变更对照表

| 旧命名 | 新命名 | 影响文件 | 变更类型 |
|---|---|---|---|
| `IOT_SENSOR_SMOKE_MV` | `IOT_SENSOR_SOIL_MV` | mqtt_iot_protocol.h, bsp_iot_controller.c, app_garden.c | 枚举重命名 |
| `IOT_SENSOR_RAIN_MV` | `IOT_SENSOR_LIGHT_MV` | mqtt_iot_protocol.h, bsp_iot_controller.c, app_garden.c | 枚举重命名 |
| (新增) | `IOT_SENSOR_RAIN_ADC_MV=5` | mqtt_iot_protocol.h, bsp_iot_controller.h/c, app_garden.c, sensor_adc_task.c | 新增枚举 |
| `iot_sensor_state_t.smoke_mv` | `soil_mv` | bsp_iot_controller.h, bsp_iot_controller.c | 字段重命名 |
| `iot_sensor_state_t.rain_mv` | `light_mv` | bsp_iot_controller.h, bsp_iot_controller.c | 字段重命名 |
| (新增) | `rain_adc_mv` | bsp_iot_controller.h, bsp_iot_controller.c, bsp_garden.h, app_garden.c | 新增字段 |
| `garden_light_state_t` | `garden_switch_state_t` | bsp_garden.h, app_garden.h, app_garden.c, garden_mcp_tool.cpp | 类型重命名 |
| `GARDEN_LIGHT_OFF` | `GARDEN_SWITCH_OFF` | bsp_garden.h, app_garden.c, garden_mcp_tool.cpp | 枚举值重命名 |
| `GARDEN_LIGHT_ON` | `GARDEN_SWITCH_ON` | bsp_garden.h, app_garden.c, garden_mcp_tool.cpp | 枚举值重命名 |
| `device_onoff_enum_t` | `device_switch_state_t` | iot_control_task.h, iot_control_task.c, sensor_adc_task.c, mqtt_receive.c | 类型重命名 |
| `OFF` | `SWITCH_OFF` | iot_control_task.h/c, sensor_adc_task.c, mqtt_receive.c | 枚举值重命名 |
| `ON` | `SWITCH_ON` | iot_control_task.h/c, sensor_adc_task.c, mqtt_receive.c | 枚举值重命名 |
| `soil_value` | `soil_percent` | bsp_garden.h, app_garden.c | 字段重命名 |
| `light_value` | `light_percent` | bsp_garden.h, app_garden.c | 字段重命名 |
| `RAIN_THRESHOLD_LIGHT` | `RAIN_THRESHOLD_MV` | sensor_config.h, sensor_adc_task.c | 宏重命名 |
| `LIGHT_1_CHINESE` | `LIGHT_COURTYARD_WEST_NAME` | iot_control_task.h | 宏重命名 |
| `LIGHT_2_CHINESE` | `LIGHT_STRIP_NAME` | iot_control_task.h | 宏重命名 |
| `LIGHT_3_CHINESE` | `LIGHT_CORRIDOR_LONG_NAME` | iot_control_task.h | 宏重命名 |
| `RELAY_1_CHINESE` | `RELAY_FAN_NAME` | iot_control_task.h | 宏重命名 |
| `RELAY_2_CHINESE` | `RELAY_WATER_PUMP_NAME` | iot_control_task.h | 宏重命名 |
| `RELAY_3_CHINESE` | `RELAY_ATOMIZER_NAME` | iot_control_task.h | 宏重命名 |
| `RELAY_4_CHINESE` | `RELAY_SAFETY_SWITCH_NAME` | iot_control_task.h | 宏重命名 |
| `SERVO_1_CHINESE` | `SERVO_CLOTHES_NAME` | iot_control_task.h | 宏重命名 |
| `servo_angle_limits_t` | `servo_angle_limit_t` | iot_control_task.h | 类型重命名 |
| (魔术数字6) | `SERVO_GPIO_INDEX_BASE=6` | app_garden.c | 宏新增 |
| `ALARM_STATE_SNOOZED=3` | (已删除) | alarm_mcp_tool.h | 枚举值删除 |
| `snooze_count`/`snooze_minutes` | (已删除) | alarm_mcp_tool.h/cpp | 字段删除 |
| `Alarm_Mcp_Snooze()` | (已删除) | alarm_mcp_tool.h/cpp | 函数删除 |
| `Garden_Mcp_Init()`/`Garden_Mcp_Start()` | (已删除) | garden_mcp_tool.h/cpp | 函数删除 |
| `g_i2c_mutex` | (已删除) | custom_esp32s3_cam.cc | 变量删除 |
| `App_ASR_Send_Emergency_Alert()` | (已删除) | app_asr_uart.h/c | 函数删除 |
| `ASR_CMD_EMERGENCY` | (已删除) | app_asr_uart.h | 宏删除 |
| `IOT_Add_Local_GPIO()` | (已删除) | bsp_iot_controller.h | 函数声明删除 |
| `atomic_flag_exchange()` | (已删除) | iot_control_task.c | 函数删除 |
| `g_fire_detected`/`g_rain_alert_triggered` | (改用xTaskNotify) | iot_control_task.c/h | 机制变更 |

---

## 5.5 Bug 修复验证清单

| 编号 | Bug描述 | 修复方案 | 验证方法 |
|---|---|---|---|
| S1 | `atomic_flag_exchange()` 非原子操作 | 改用 `xTaskNotify/xTaskNotifyFromISR` | 编译通过 + 代码审查 |
| S2 | volatile与互斥锁混用 | 删除 `g_fire_detected`/`g_rain_alert_triggered`，保留 `g_help_detected` | 编译通过 + 代码审查 |
| S5 | 传感器映射错误 | 枚举/字段重命名 | 编译通过 + 映射表验证 |
| S6 | 雨天检测夜间误报 | 新增独立 `RAIN_ADC_CHANNEL` | 编译通过 + 代码审查 |
| S7 | 贪睡到期不重新触发 | 直接移除贪睡功能 | 编译通过 + 代码审查 |
| M1 | Broadcast_Lights_On不更新状态缓存 | 添加 `gpio_states` 更新 | 编译通过 + 代码审查 |
| M2 | 火灾双重广播 | 删除直接 publish | 编译通过 + 代码审查 |
| M11 | 心跳间隔=keepalive | 改为 10s | 编译通过 + 代码审查 |
| M12 | uint8_t溢出 | 改为 uint16_t + 上限保护 | 编译通过 + 代码审查 |
| M13 | 自动控制覆盖远程指令 | 30s冷却期机制 | 编译通过 + 代码审查 |
| L14 | MQTT数据包对齐 | memcpy到局部变量 | 编译通过 + 代码审查 |
| L16 | 命令码缺乏注释 | 添加分区注释 | 代码审查 |

---

## 5.6 实施顺序

```
步骤1: 遗留代码修复
  ├── 5.2.1 bsp_garden.c 空文件处理
  ├── 5.2.2 iot_controller.c 本地GPIO清理
  └── 5.2.3 LVGL仿真层命名一致性

步骤2: 从机工程编译验证
  └── idf.py build (slave)

步骤3: 主机工程编译验证
  └── idf.py build (main)

步骤4: 编译错误修复（如有）
  └── 根据编译输出逐一修复

步骤5: 文档整理
  ├── 传感器映射关系文档（更新到本计划文件）
  └── 命名变更对照表（已整理在本计划文件中）
```

---

## 5.7 风险评估

| 风险项 | 概率 | 影响 | 缓解措施 |
|---|---|---|---|
| 从机编译失败（RAIN_ADC_CHANNEL GPIO冲突） | 中 | 高 | 检查 sdkconfig 和板级配置，确认 GPIO4 可用 |
| 主机编译失败（C++/C混合链接问题） | 低 | 中 | 确认 extern "C" 包裹正确 |
| NVS旧数据加载失败（alarm结构体变更） | 低 | 低 | 已有容错处理（nvs_get_blob长度不匹配） |
| LVGL仿真编译失败（字段重命名遗漏） | 低 | 低 | 仿真层独立编译，不影响主工程 |
