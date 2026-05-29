# 电表数据采集功能 - 可行性分析与实施计划

## 一、可行性分析

### 1.1 硬件可行性 ✅ 完全可行

| 评估项 | 结论 | 说明 |
|--------|------|------|
| 电流/电压传感器 | **不需要** | 采用额定功率×运行时间的纯软件方案 |
| NVS 存储 | **已有** | ESP32 从机已初始化 NVS（`nvs_flash_init()`），存储空间充足 |
| RTC 时钟 | **可选** | 使用 `esp_timer`（硬件定时器）计算相对运行时间，不依赖绝对时间 |
| MQTT 通信 | **已有** | 从机已有完整的 MQTT 连接和发布能力 |

### 1.2 软件可行性 ✅ 完全可行

| 评估项 | 结论 | 说明 |
|--------|------|------|
| 继电器状态追踪 | **已有** | `g_device_flags.light[]` 和 `g_device_flags.relay[]` 已实时反映设备状态 |
| 状态变化检测 | **需新增** | 在 `Device_Refresh_Task` 100ms 循环中检测状态边沿变化 |
| MQTT Topic 扩展 | **简单** | 新增 `xiaozhi/iot/energy/{MAC}` Topic，复用现有 `esp_mqtt_client_publish()` |
| NVS 读写 | **简单** | 使用 `nvs_open/nvs_get_blob/nvs_set_blob` 持久化 kWh 累计值 |

### 1.3 精度分析

| 设备 | 额定功率 | 24h 满载用电 | float 精度(23bit尾数) | 精度是否足够 |
|------|----------|-------------|---------------------|-------------|
| 西庭灯 | 10W | 0.240 kWh | 7位有效数字 | ✅ 足够 |
| 灯带 | 15W | 0.360 kWh | 7位有效数字 | ✅ 足够 |
| 长廊灯 | 12W | 0.288 kWh | 7位有效数字 | ✅ 足够 |
| 风扇 | 35W | 0.840 kWh | 7位有效数字 | ✅ 足够 |
| 水泵 | 25W | 0.600 kWh | 7位有效数字 | ✅ 足够 |
| 雾化器 | 15W | 0.360 kWh | 7位有效数字 | ✅ 足够 |

> 结论：float 类型完全满足需求。即使连续运行 1 年（~8760h），最大累计值约 306 kWh，float 精度仍可精确到 0.001 kWh。

### 1.4 关键设计决策

| 决策点 | 方案 | 理由 |
|--------|------|------|
| 时间计量方式 | `esp_timer_get_time()`（微秒级硬件定时器） | 不依赖 NTP/RTC，掉电不丢失计时精度，重启后从 0 开始 |
| kWh 积分时机 | 每 100ms 循环中实时积分 | 避免"关闭时才积分"导致的断电丢失风险 |
| NVS 刷新频率 | 每 30 秒持久化一次 | 平衡 Flash 寿命（NVS 写入次数有限）和数据安全性 |
| 上报时机 | 状态变化 + 2小时周期 + 启动时 | 与协议文档 v2.1 一致 |
| 安防总闸 | 不纳入电量统计 | 总闸是供电开关，不是用电设备 |

### 1.5 风险点

| 风险 | 影响 | 缓解方案 |
|------|------|----------|
| NVS Flash 磨损 | 每 30s 写入，Flash 擦写次数约 10 万次 → 约 34 天 | ① 降低刷新频率到 5 分钟 ② 或使用 NVS 的 page rotation 机制 |
| 断电数据丢失 | 最多丢失 5 分钟的积分数据 | 可接受（0.003 kWh 量级误差） |
| 原协议文档用 `ESP_ERR_NVS_*` 但项目是 ESP32 非 ESP32-S3 | 无影响 | NVS API 在 ESP32/ESP32-S3 上完全一致 |

---

## 二、需要新增/修改的文件清单

### 从机端（gardenslave）

| 文件 | 操作 | 说明 |
|------|------|------|
| `main/APP/Inc/energy_tracker.h` | **新增** | 能耗追踪模块头文件 |
| `main/APP/Src/energy_tracker.c` | **新增** | 能耗追踪模块实现 |
| `main/APP/Inc/mqtt_receive.h` | **修改** | 添加 `MQTT_Publish_Energy()` 声明 |
| `main/APP/Src/mqtt_receive.c` | **修改** | 实现 `MQTT_Publish_Energy()` |
| `main/APP/Src/iot_control_task.c` | **修改** | 在 `Device_Refresh_Task` 中集成能耗积分 |
| `main/iot_slave_main.c` | **修改** | 初始化能耗追踪模块 |
| `main/CMakeLists.txt` | **修改** | 添加 `energy_tracker.c` 源文件 |
| `README.md` | **修改** | 添加能耗功能文档 |

### 共享协议

| 文件 | 操作 | 说明 |
|------|------|------|
| `../../../shared/mqtt_iot_protocol.h` | **修改** | 添加能耗相关 Topic 和设备索引定义 |

---

## 三、详细实施步骤

### 步骤 1：扩展 MQTT 协议定义

**文件**: `shared/mqtt_iot_protocol.h`

新增内容：
```c
/* ================= 能耗追踪 ================= */
#define MQTT_TOPIC_ENERGY_PREFIX    "xiaozhi/iot/energy/"
#define MQTT_TOPIC_ENERGY_REQ_PREFIX "xiaozhi/iot/energy/request/"

/* 设备能耗索引（对应 energy_tracker 中的数组下标） */
typedef enum {
    ENERGY_DEV_CORRIDOR_LIGHT = 0,  // 西庭灯  10W
    ENERGY_DEV_STRIP_LIGHT    = 1,  // 庭院灯带 15W
    ENERGY_DEV_VEG_LIGHT      = 2,  // 长廊灯  12W
    ENERGY_DEV_FAN            = 3,  // 通风扇  35W
    ENERGY_DEV_WATER_PUMP     = 4,  // 水泵    25W
    ENERGY_DEV_ATOMIZER       = 5,  // 雾化器  15W
    ENERGY_DEV_COUNT          = 6
} energy_device_index_t;

#define ENERGY_RATE_W_CORRIDOR_LIGHT  10
#define ENERGY_RATE_W_STRIP_LIGHT     15
#define ENERGY_RATE_W_VEG_LIGHT       12
#define ENERGY_RATE_W_FAN             35
#define ENERGY_RATE_W_WATER_PUMP      25
#define ENERGY_RATE_W_ATOMIZER        15
```

### 步骤 2：实现能耗追踪模块

**文件**: `main/APP/Inc/energy_tracker.h`

```c
#ifndef ENERGY_TRACKER_H
#define ENERGY_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

#define ENERGY_DEV_COUNT    6
#define ENERGY_NVS_NAMESPACE "energy"
#define ENERGY_NVS_KEY       "accum"
#define ENERGY_SAVE_INTERVAL_MS  300000  // 5分钟持久化一次（保护Flash）
#define ENERGY_REPORT_INTERVAL_S 7200   // 2小时兜底上报

typedef struct {
    float kwh[ENERGY_DEV_COUNT];
} energy_accum_t;

void Energy_Tracker_Init(void);
void Energy_Tracker_Update(bool device_on[ENERGY_DEV_COUNT]);
float Energy_Tracker_Get_Total(void);
const energy_accum_t* Energy_Tracker_Get_Data(void);
void Energy_Tracker_Report_Now(void);

#endif
```

**文件**: `main/APP/Src/energy_tracker.c`

核心逻辑：
1. `Energy_Tracker_Init()` - 从 NVS 恢复累计值
2. `Energy_Tracker_Update()` - 在 100ms 循环中调用，根据设备 ON 状态积分
3. 使用 `esp_timer_get_time()` 计算两次调用的时间差
4. 每 5 分钟自动持久化到 NVS
5. 每 2 小时自动触发 MQTT 上报
6. `Energy_Tracker_Report_Now()` - 立即上报（状态变化时调用）

积分公式：
```c
delta_kwh = (rated_watts[device] / 1000.0f) * (delta_us / 3600000000.0f);
accum.kwh[device] += delta_kwh;
```

### 步骤 3：实现 MQTT 能耗上报

**文件**: `main/APP/Src/mqtt_receive.c`（修改）

新增函数 `MQTT_Publish_Energy()`：
```c
void MQTT_Publish_Energy(const energy_accum_t* data, const bool on_state[ENERGY_DEV_COUNT]) {
    if (!MQTT_Can_Publish("energy")) return;

    float total = 0;
    for (int i = 0; i < ENERGY_DEV_COUNT; i++) total += data->kwh[i];

    char buf[512];
    snprintf(buf, sizeof(buf),
        "{"
          "\"devices\":{"
            "\"corridor_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"strip_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"veg_light\":{\"kwh\":%.3f,\"on\":%s},"
            "\"fan\":{\"kwh\":%.3f,\"on\":%s},"
            "\"water_pump\":{\"kwh\":%.3f,\"on\":%s},"
            "\"atomizer\":{\"kwh\":%.3f,\"on\":%s}"
          "},"
          "\"total_kwh\":%.3f"
        "}",
        data->kwh[0], on_state[0] ? "true" : "false",
        data->kwh[1], on_state[1] ? "true" : "false",
        data->kwh[2], on_state[2] ? "true" : "false",
        data->kwh[3], on_state[3] ? "true" : "false",
        data->kwh[4], on_state[4] ? "true" : "false",
        data->kwh[5], on_state[5] ? "true" : "false",
        total
    );

    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_ENERGY_PREFIX "%s", MQTT_Get_MAC_Str());
    esp_mqtt_client_publish(MQTT_Get_Client(), topic, buf, 0, 1, 1);  // QoS 1, retain
}
```

### 步骤 4：集成到设备控制循环

**文件**: `main/APP/Src/iot_control_task.c`（修改）

在 `Device_Refresh_Task` 的 100ms 循环中：

```c
while (1) {
    device_flags_enum_t flags_snapshot;
    DEVICE_FLAGS_LOCK();
    flags_snapshot = g_device_flags;
    DEVICE_FLAGS_UNLOCK();

    // ... 现有的 GPIO 刷新逻辑 ...

    // 能耗积分（每 100ms 调用一次）
    bool energy_dev_on[ENERGY_DEV_COUNT] = {
        flags_snapshot.light[0] == SWITCH_ON,   // corridor_light
        flags_snapshot.light[1] == SWITCH_ON,   // strip_light
        flags_snapshot.light[2] == SWITCH_ON,   // veg_light
        flags_snapshot.relay[0] == SWITCH_ON,   // fan
        flags_snapshot.relay[1] == SWITCH_ON,   // water_pump
        flags_snapshot.relay[2] == SWITCH_ON    // atomizer
    };
    Energy_Tracker_Update(energy_dev_on);

    vTaskDelay(pdMS_TO_TICKS(REFRESH_INTERVAL_MS));
}
```

### 步骤 5：初始化能耗追踪

**文件**: `main/iot_slave_main.c`（修改）

在 `MQTT_Receive_Init()` 之后添加：
```c
#include "energy_tracker.h"

// ... 在 app_main() 中 ...
ESP_LOGI(TAG, "Initializing Energy Tracker...");
Energy_Tracker_Init();
```

### 步骤 6：更新构建系统

**文件**: `main/CMakeLists.txt`（修改）

```cmake
idf_component_register(SRCS "iot_slave_main.c"
                            "APP/Src/mqtt_receive.c"
                            "APP/Src/mqtt_heartbeat.c"
                            "APP/Src/iot_control_task.c"
                            "APP/Src/app_ws2812_strip.c"
                            "APP/Src/sensor_adc_task.c"
                            "APP/Src/app_wifi.c"
                            "APP/Src/app_asr_uart.c"
                            "APP/Src/energy_tracker.c"     # 新增
                            "BSP/Src/bsp_wifi.c"
                            "BSP/Src/bsp_asr_uart.c"
                       PRIV_REQUIRES esp_wifi nvs_flash driver esp_netif esp_event esp_adc mqtt led_strip
                       INCLUDE_DIRS "." "APP/Inc" "BSP/Inc" "../../../shared")
```

### 步骤 7：更新 README 文档

**文件**: `slave/gardenslave/README.md`（修改）

在"功能特性"部分新增"能耗追踪"章节，包括：
- 功能描述
- MQTT Topic 和数据格式
- 设备额定功率表
- NVS 持久化说明

---

## 四、与协议文档 v2.1 的差异说明

| 协议文档要求 | 实际实现 | 差异原因 |
|-------------|----------|----------|
| `ts` 字段（ISO8601 时间戳） | **省略** | 从机无 NTP/RTC，无法获取绝对时间。App 可用收到消息的时间代替 |
| 每次继电器动作时立即上报 | **状态变化时上报 + 2 小时兜底** | 避免场景切换（7 种场景）时频繁上报 |
| NVS 每次动作后立即持久化 | **每 5 分钟持久化** | 保护 Flash 寿命（ESP32 NVS 擦写约 10 万次） |
| 设备额定功率常量 | **已定义为宏** | 可通过修改宏调整，无需改逻辑 |

---

## 五、测试验证计划

### 5.1 单元验证
- [ ] NVS 读写：写入 → 重启 → 读出，确认数据一致
- [ ] 积分精度：模拟设备运行 1 小时，验证 kWh 值误差 < 1%
- [ ] 上报格式：用 MQTT 客户端订阅，验证 JSON 格式正确

### 5.2 集成验证
- [ ] 设备开关 → 触发立即上报
- [ ] 重启后 → 从 NVS 恢复累计值并上报
- [ ] 2 小时无操作 → 自动兜底上报
- [ ] App 端订阅 → 解析并显示数据

### 5.3 边界条件
- [ ] 首次启动（NVS 为空）→ 全部为 0
- [ ] 火灾紧急响应 → 设备状态变化后正常积分
- [ ] 断电重启 → 最多丢失 5 分钟数据

---

## 六、实施优先级

| 优先级 | 步骤 | 预计工作量 |
|--------|------|-----------|
| P0 | 步骤 1-2：协议定义 + 能耗追踪模块 | 核心功能 |
| P0 | 步骤 3-4：MQTT 上报 + 设备控制集成 | 核心功能 |
| P1 | 步骤 5-6：初始化 + 构建系统 | 必要 |
| P2 | 步骤 7：README 文档更新 | 文档 |
