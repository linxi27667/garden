# MCU 用电数据采集与上报协议 v2.1

## 0. 设计决策

### 为什么不用瞬时功率传感器？

现有 MCU 硬件无电流/电压采样电路。所有 kWh 计算基于：
```
kWh = (设备额定功率W / 1000) × 该设备累计运行小时数
```

### 设备额定功率（常量，MCU 和 App 两侧保持一致）

| 设备 | device_id | 额定功率 W | 说明 |
|------|-----------|-----------|------|
| 西庭灯 | `corridor_light` | 10 | 小黄色 LED |
| 庭院灯带 | `strip_light` | 15 | WS2812B × 30 颗 |
| 长廊灯 | `veg_light` | 12 | 暖黄 LED 灯带 |
| 通风扇 | `fan` | 35 | 排风扇继电器 |
| 水泵 | `water_pump` | 25 | 抽水泵继电器 |
| 雾化器 | `atomizer` | 15 | 雾化器继电器 |

> ⚠️ 这些功率值需你根据实际硬件确认。MCU 侧应做成可配置常量。

---

## 1. MQTT 上报协议

### 1.1 Topic

```
xiaozhi/iot/energy/{MAC}
```

- QoS 1
- **Retained = true**（重要：App 订阅时立刻拿到最新值）
- `{MAC}` 为 MCU 的 Wi-Fi MAC 地址（与现有心跳 Topic 规则一致）

### 1.2 上报时机

| 触发条件 | 说明 |
|---------|------|
| **设备状态变化** | 任意设备继电器开/关时，立即上报 |
| **定期上报** | 即使无变化，每 2 小时上报一次 |
| **MCU 启动** | 从 NVS 恢复累计值后立即上报 |
| **App 请求** | 收到 `xiaozhi/iot/energy/request/{MAC}` 时回复 |

### 1.3 JSON Payload

```json
{
  "ts": "2026-05-27T14:35:22",
  "devices": {
    "corridor_light":  { "kwh": 0.455,  "on": true },
    "strip_light":     { "kwh": 0.453,  "on": true },
    "veg_light":       { "kwh": 0.240,  "on": false },
    "fan":             { "kwh": 1.253,  "on": true },
    "water_pump":      { "kwh": 0.375,  "on": false },
    "atomizer":        { "kwh": 0.158,  "on": true }
  },
  "total_kwh": 2.934
}
```

### 1.4 字段说明

| 路径 | 类型 | 说明 |
|------|------|------|
| `ts` | ISO8601 | 当前时间戳 |
| `devices.{id}.kwh` | float | **累计用电量**（自 MCU 出厂/手动复位以来，永远只增不减） |
| `devices.{id}.on` | bool | 当前继电器通断状态 |
| `total_kwh` | float | 6 设备累计值加总 |

### 1.5 关键设计原则

**MCU 只负责累加，App 负责计算费用和月度差值。**
- MCU 不知道今天是几月几号，不用操心"月度清零"
- MCU 每次继电器动作后立即积分并持久化到 NVS
- App 拿到 `kwh` 值后用 `本月用电 = 当前kwh - 月初kwh` 算增量

---

## 2. App 接入方案

### 2.1 数据流

```
MCU 继电器动作 → 计算 kWh 增量 → 写入 NVS → MQTT publish (retained)
                                                    ↓
App 打开能源页面 → MQTT subscribe → 拿到 retained 消息 → 显示数据
                                                    ↓
App 持续订阅 → 设备状态变化触发新消息 → UI 实时刷新
```

### 2.2 1 分钟刷新策略

App 侧不需要定时器轮询。因为：
1. MCU 在**每次设备开关时**已立即上报
2. 如果设备状态没有变化，kWh 值不会变（运行时积分在 MCU 内部完成，不需要 App 参与）
3. App 可以做**显示层插值**：`估算kWh = 上次上报kWh + 额定功率/1000 × (当前时间 - 上报时间) × ON状态`

### 2.3 月度用电计算

```
App 存储在本地：
- month_start_kwh[device_id]  ← 月初零点时记录的基准 kWh

本月某设备用电 = 当前MCU上报的 kwh - month_start_kwh
本月总用电     = Σ 所有设备本月用电
本月总费用     = 本月总用电 × 0.5988

月度重置：每月 1 日 00:00 由 App 自动更新 month_start_kwh
```

### 2.4 24h 图表数据

MCU 每 2 小时上报一次。App 将连续两个 2h 上报的 `kwh` 值相减，得到该 2 小时窗口内的用电增量：

```
slot[12:00] = kwh_at_14:00 - kwh_at_12:00
```

如果 App 因后台被杀错过了某次上报，在订阅时拿到的最新 retained 消息可以作为当前基准。

---

## 3. App 侧代码改动清单

| 文件 | 改动 |
|------|------|
| `MqttConfig.kt` | 增加 `TOPIC_XIAOZHI_ENERGY` 常量 |
| `DashboardViewModel.kt` | 增加能量数据 StateFlow + MQTT 订阅 + 解析 |
| `EnergyScreen.kt` | 绑定 ViewModel 真实数据 + 移除 mock |
| `data/model/Models.kt` | 增加能量数据模型类 |

---

## 4. MCU 端实现伪代码

```c
#include <time.h>
#include "nvs_flash.h"
#include "esp_timer.h"

// ── 设备额定功率（瓦）──
#define RATED_W_CORRIDOR_LIGHT  10
#define RATED_W_STRIP_LIGHT     15
#define RATED_W_VEG_LIGHT       12
#define RATED_W_FAN             35
#define RATED_W_WATER_PUMP      25
#define RATED_W_ATOMIZER        15

// 6 设备的累计 kWh，持久化到 NVS
typedef struct {
    float corridor_light;
    float strip_light;
    float veg_light;
    float fan;
    float water_pump;
    float atomizer;
} energy_accum_t;

static energy_accum_t g_energy;
static uint8_t g_relay_state[6];  // 当前继电器状态

// ── 继电器动作时调用，index: 0=西庭灯 1=灯带 2=长廊灯 3=通风扇 4=水泵 5=雾化器 ──
void on_relay_changed(int index, bool new_state, uint32_t runtime_sec) {
    bool old_state = g_relay_state[index];
    g_relay_state[index] = new_state;

    if (old_state && !new_state) {
        // 设备关闭：将本次运行时长积分到 kWh
        float rated_w[6] = {10, 15, 12, 35, 25, 15};
        float kwh_add = (rated_w[index] / 1000.0f) * (runtime_sec / 3600.0f);
        add_kwh(index, kwh_add);
    }

    save_to_nvs();     // 立即持久化
    publish_energy();  // 立即上报
}

static void add_kwh(int index, float kwh) {
    float *p[] = {&g_energy.corridor_light, &g_energy.strip_light,
                  &g_energy.veg_light, &g_energy.fan,
                  &g_energy.water_pump, &g_energy.atomizer};
    *p[index] += kwh;
}

static void publish_energy(void) {
    float total = g_energy.corridor_light + g_energy.strip_light
                + g_energy.veg_light + g_energy.fan
                + g_energy.water_pump + g_energy.atomizer;

    char buf[512];
    snprintf(buf, sizeof(buf),
        "{"
          "\"ts\":\"%s\","
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
        get_iso8601_now(),
        g_energy.corridor_light, g_relay_state[0] ? "true" : "false",
        g_energy.strip_light,    g_relay_state[1] ? "true" : "false",
        g_energy.veg_light,      g_relay_state[2] ? "true" : "false",
        g_energy.fan,            g_relay_state[3] ? "true" : "false",
        g_energy.water_pump,     g_relay_state[4] ? "true" : "false",
        g_energy.atomizer,       g_relay_state[5] ? "true" : "false",
        total
    );
    mqtt_publish("xiaozhi/iot/energy/" DEVICE_MAC, buf, /* qos */ 1, /* retain */ true);
}

// ── 每 2 小时兜底上报 ──
void periodic_energy_report(void) {
    static uint32_t last_report = 0;
    if (esp_timer_get_time() - last_report > 7200 * 1000000ULL) {
        publish_energy();
        last_report = esp_timer_get_time();
    }
}

// ── MCU 启动时从 NVS 恢复 ──
void energy_init(void) {
    nvs_handle_t handle;
    nvs_open("energy", NVS_READWRITE, &handle);
    size_t sz = sizeof(g_energy);
    if (nvs_get_blob(handle, "accum", &g_energy, &sz) != ESP_OK) {
        memset(&g_energy, 0, sizeof(g_energy));  // 首次启动
    }
    nvs_close(handle);
    publish_energy();  // 恢复后上报当前累计值
}
```

---

## 5. App 侧重要实现细节

### 5.1 电量解析

```kotlin
// DashboardViewModel.kt 中新增
private val _energyData = MutableStateFlow<EnergyData?>(null)
val energyData: StateFlow<EnergyData?> = _energyData.asStateFlow()

private fun parseEnergyData(message: String) {
    val json = JSONObject(message)
    val totalKwh = json.getDouble("total_kwh").toFloat()
    val devices = json.getJSONObject("devices")
    val perDevice = devices.keys().asSequence().map { deviceId ->
        val obj = devices.getJSONObject(deviceId)
        EnergyDevice(
            id = deviceId,
            kwh = obj.getDouble("kwh").toFloat(),
            isOn = obj.getBoolean("on")
        )
    }.toList()
    _energyData.value = EnergyData(
        timestamp = json.getString("ts"),
        totalKwh = totalKwh,
        devices = perDevice
    )
}
```

### 5.2 MQTT 订阅

```kotlin
// DashboardViewModel.kt init 中新增
mqttManager.subscribe(MqttConfig.TOPIC_XIAOZHI_ENERGY) { parseEnergyData(it) }
```

### 5.3 月度用电计算

```kotlin
// 存储在 SharedPreferences / DataStore
// key: "month_start_kwh_2026-06" → 2026年6月的月初基准
val monthKey = "month_start_kwh_${year}_${month}"
val startKwh = prefs.getFloat(monthKey, -1f)
if (startKwh < 0) {
    // 新月份：记录当前 kwh 作为基准
    prefs.edit().putFloat(monthKey, currentTotalKwh).apply()
    monthUsage = 0f
} else {
    monthUsage = currentTotalKwh - startKwh
}
```
