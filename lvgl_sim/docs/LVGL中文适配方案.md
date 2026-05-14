# LVGL 中文字体适配完整方案

> 本文档详细说明如何在 LVGL v9 项目中实现中文字体适配，采用最严谨的方案确保中文显示正确。

---

## 一、核心原理

### 1.1 LVGL 字体系统概述

LVGL 支持两种字体加载方式：

| 方式 | 说明 | 适用场景 |
|------|------|----------|
| **内置字体 (Built-in)** | 编译时打包，固定字形 | 英文、数字、简单符号 |
| **动态字体 (Tiny-TTF)** | 运行时加载 TTF/OTC 文件 | 中文、日文等多字符集语言 |

### 1.2 为什么选择 Tiny-TTF？

- 内置字体无法包含所有汉字（GB2312 有 6763 个汉字，UTF-8 中文字符更多）
- Tiny-TTF 使用 `lv_tiny_ttf_create_data()` 从内存加载字体数据
- 字体文件独立管理，可按需更新，无需重新编译

---

## 二、完整配置步骤

### 2.1 启用 Tiny-TTF 插件

在 `lv_conf.h` 中添加以下配置：

```c
#define LV_USE_TINY_TTF             1
#define LV_TINY_TTF_FILE_SUPPORT    0
```

> **注意**：`LV_TINY_TTF_FILE_SUPPORT` 设为 0 表示从内存加载，避免 SD 卡依赖。

### 2.2 启用二进制字体支持（可选，用于英文图标）

```c
#define LV_USE_BINFONT               1
```

### 2.3 配置内置字体

```c
#define LV_FONT_MONTSERRAT_12       1
#define LV_FONT_MONTSERRAT_14       1
#define LV_FONT_MONTSERRAT_16       1
#define LV_FONT_MONTSERRAT_20       1
#define LV_FONT_MONTSERRATT_24      1

#define LV_FONT_DEFAULT              &lv_font_montserrat_14
```

### 2.4 确保文件系统支持

```c
#define LV_USE_FS_WIN32         1
#define LV_FS_WIN32_LETTER      'C'
```

---

## 三、中文字体文件获取

### 3.1 推荐字体

| 字体 | 特点 | 下载地址 |
|------|------|----------|
| **Noto Sans SC** | Google 开源，简洁清晰 | https://fonts.google.com/specimen/Noto+Sans+SC |
| **Source Han Sans** | 思源黑体，Adobe/Google 联合开发 | https://github.com/adobe-fonts/source-han-sans |
| **WenQuanYi Micro Hei** | 圆润可爱，适合 UI | https://github.com/anthonyfok/fonts-wqy-micro-hei |

### 3.2 下载步骤（以 Noto Sans SC 为例）

1. 访问 https://fonts.google.com/specimen/Noto+Sans+SC
2. 点击右上角 "Download family"
3. 解压下载的 ZIP 文件
4. 找到 `NotoSansSC-Regular.ttf` 文件
5. 复制到项目 `fonts` 目录

### 3.3 字体文件命名规范

```
main/
└── UI/
    └── fonts/
        ├── NotoSansSC-Regular.ttf    ← 中文字体
        ├── fa-solid-900.ttf          ← FontAwesome 矢量图标
        ├── fa-regular-400.ttf        ← FontAwesome 常规图标
        └── ui_font.h                ← 字体管理代码
```

---

## 四、代码实现

### 4.1 字体管理头文件 (ui_font.h)

```c
#ifndef UI_FONT_H
#define UI_FONT_H

#include "lvgl/lvgl.h"

// 内置字体别名
#define UI_FONT_12      &lv_font_montserrat_12
#define UI_FONT_14      &lv_font_montserrat_14
#define UI_FONT_17      &lv_font_montserrat_14
#define UI_FONT_20      &lv_font_montserrat_20
#define UI_FONT_24      &lv_font_montserrat_24
#define UI_FONT_36      &lv_font_montserrat_36

// 用途别名
#define UI_FONT_TAB     UI_FONT_12
#define UI_FONT_SMALL   UI_FONT_12
#define UI_FONT_BODY    UI_FONT_14
#define UI_FONT_LABEL   UI_FONT_17
#define UI_FONT_TITLE   UI_FONT_20
#define UI_FONT_VALUE   UI_FONT_24
#define UI_FONT_HERO    UI_FONT_36

// 函数声明
void ui_font_init(void);
const lv_font_t *ui_font_cn(uint8_t size);
const lv_font_t *ui_font_icon(uint8_t size);

#endif
```

### 4.2 字体管理实现文件 (ui_font.c)

```c
#include "ui_font.h"
#include <stdio.h>
#include <stdlib.h>

#if LV_USE_TINY_TTF

// 中文字体缓存
static lv_font_t *s_cn_12 = NULL;
static lv_font_t *s_cn_14 = NULL;
static lv_font_t *s_cn_20 = NULL;
static lv_font_t *s_cn_24 = NULL;
static lv_font_t *s_cn_36 = NULL;

#if LV_USE_BINFONT
// 图标字体缓存
static const lv_font_t *s_fa_solid = NULL;
#endif

/**
 * 从文件加载数据的辅助函数
 */
static void *load_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        LV_LOG_WARN("ui_font: failed to open %s", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) {
        fclose(f);
        return NULL;
    }
    void *data = malloc(len);
    if (!data) {
        fclose(f);
        return NULL;
    }
    fread(data, 1, len, f);
    fclose(f);
    *out_size = len;
    return data;
}

/**
 * 字体初始化 - 在 lv_init() 之后调用
 */
void ui_font_init(void)
{
    // 1. 加载中文字体
    const char *cn_paths[] = {
        "NotoSansSC.ttf",                       // 相对路径（可执行文件目录）
        "main/UI/fonts/NotoSansSC-Regular.ttf", // 开发环境路径
    };

    size_t ttf_size = 0;
    void *ttf_data = NULL;

    // 尝试多个路径
    for (int i = 0; i < 2 && ttf_data == NULL; i++) {
        ttf_data = load_file(cn_paths[i], &ttf_size);
    }

    if (!ttf_data) {
        LV_LOG_ERROR("ui_font: NotoSansSC TTF not found!");
        LV_LOG_ERROR("ui_font: Please copy NotoSansSC-Regular.ttf to the executable directory");
    } else {
        // 从内存数据创建 Tiny-TTF 字体
        s_cn_12 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 12);
        s_cn_14 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 14);
        s_cn_20 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 20);
        s_cn_24 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 24);
        s_cn_36 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 36);
        LV_LOG_INFO("ui_font: NotoSansSC loaded successfully");
    }

    // 2. 加载图标字体（FontAwesome）
#if LV_USE_BINFONT
    const char *fa_solid_path = "fa-solid-900.ttf";
    s_fa_solid = lv_binfont_create(fa_solid_path);
    if (s_fa_solid) {
        LV_LOG_INFO("ui_font: FontAwesome Solid loaded");
    } else {
        LV_LOG_WARN("ui_font: FontAwesome not found, icons may not display");
    }
#endif
}

/**
 * 获取指定字号的中文字体
 * @param size 字号大小（12, 14, 20, 24, 36）
 * @return 字体指针，如果未加载则返回内置字体作为后备
 */
const lv_font_t *ui_font_cn(uint8_t size)
{
#if LV_USE_TINY_TTF
    switch (size) {
        case 12: return s_cn_12 ? s_cn_12 : UI_FONT_12;
        case 14: return s_cn_14 ? s_cn_14 : UI_FONT_14;
        case 20: return s_cn_20 ? s_cn_20 : UI_FONT_20;
        case 24: return s_cn_24 ? s_cn_24 : UI_FONT_24;
        case 36: return s_cn_36 ? s_cn_36 : UI_FONT_36;
        default: return s_cn_14 ? s_cn_14 : UI_FONT_14;
    }
#else
    LV_LOG_WARN("ui_font_cn: LV_USE_TINY_TTF is disabled");
    return UI_FONT_14;
#endif
}

/**
 * 获取图标字体
 */
const lv_font_t *ui_font_icon(uint8_t size)
{
    (void)size;
#if LV_USE_BINFONT
    return s_fa_solid ? s_fa_solid : UI_FONT_24;
#else
    return UI_FONT_24;
#endif
}

#else  // LV_USE_TINY_TTF

void ui_font_init(void)
{
    LV_LOG_WARN("ui_font_init: LV_USE_TINY_TTF is disabled, Chinese text may not display");
}

const lv_font_t *ui_font_cn(uint8_t size)
{
    (void)size;
    return UI_FONT_14;
}

const lv_font_t *ui_font_icon(uint8_t size)
{
    (void)size;
    return UI_FONT_24;
}

#endif  // LV_USE_TINY_TTF
```

---

## 五、i18n 国际化中的字体适配

### 5.1 i18n 字体切换逻辑

```c
const lv_font_t *ui_i18n_font(const lv_font_t *fallback)
{
    if (s_lang == UI_LANG_ZH) {
        // 中文使用 Noto Sans SC
        return ui_font_cn(14);
    }
    // 英文使用内置字体
    return fallback;
}
```

### 5.2 标签创建辅助函数

```c
static lv_obj_t *mk_label(lv_obj_t *parent, const char *text,
                          const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    // 关键：使用 i18n 字体函数，自动选择中文字体
    lv_obj_set_style_text_font(lbl, ui_i18n_font(font), 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}
```

---

## 六、CMake 构建配置

### 6.1 复制字体到构建目录

```cmake
# 在 CMakeLists.txt 中添加
add_custom_command(TARGET garden_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/main/UI/fonts/NotoSansSC-Regular.ttf"
        "${CMAKE_BINARY_DIR}/NotoSansSC.ttf"
    COMMENT "Copy Chinese TTF font to build directory")

add_custom_command(TARGET garden_sim POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/main/UI/fonts/fa-solid-900.ttf"
        "${CMAKE_BINARY_DIR}/fa-solid-900.ttf"
    COMMENT "Copy FontAwesome font to build directory")
```

### 6.2 确保字体文件在运行时可访问

```
构建输出目录/
├── garden_sim.exe          ← 主程序
├── NotoSansSC.ttf          ← 必须存在
├── fa-solid-900.ttf        ← 图标字体
└── fa-regular-400.ttf      ← 图标字体
```

---

## 七、常见问题排查

### 问题 1：中文字体完全不显示

**排查步骤：**
1. 检查 `lv_conf.h` 中 `LV_USE_TINY_TTF` 是否为 1
2. 检查字体文件是否在可执行文件同目录
3. 检查日志输出是否有 "NotoSansSC loaded" 字样
4. 检查 `lv_tiny_ttf_create_data()` 返回值是否为 NULL

**解决方法：**
```bash
# 检查字体文件是否存在
dir NotoSansSC.ttf

# 检查文件大小
ls -la NotoSansSC.ttf
# 正常的中文字体文件应该 > 1MB
```

### 问题 2：部分汉字显示为方块（豆腐）

**原因：** 字体文件中不包含该汉字的字形

**解决方法：**
- 使用更完整的字体（如思源黑体）
- 下载完整版而非精简版

### 问题 3：字体加载后应用变慢

**原因：** Tiny-TTF 每次渲染汉字都需要从内存解析

**优化方案：**
- 使用 LVGL 字体缓存：`lv_cache_set_size(LV_CACHE_SIZE_ALL, 64)`
- 限制应用的中文字体大小种类

### 问题 4：PC 模拟器正常，嵌入式设备不显示中文

**排查步骤：**
1. 检查嵌入式系统的文件系统是否正确挂载字体文件
2. 检查 `LV_FS_WIN32` 相关配置是否已移除或正确配置嵌入式文件系统
3. 检查内存是否足够（TTF 文件需要完整加载到内存）

**嵌入式设备建议：**
```c
// 在嵌入式环境中使用绝对路径
const char *cn_paths[] = {
    "/sdcard/fonts/NotoSansSC.ttf",  // SD 卡路径
    "/flash/fonts/NotoSansSC.ttf",    // Flash 路径
};
```

---

## 八、进阶优化

### 8.1 字体子集化（减少体积）

使用 `pyftsubset` 工具只包含需要的汉字：

```bash
pip install fonttools

pyftsubset NotoSansSC-Regular.ttf \
    --text="数据控制设置在线智能菜园ESP32温度湿度土壤光照" \
    --output-file=NotoSansSC-subset.ttf
```

### 8.2 混合字体方案

将常用汉字打包为内置字体，偶发字符使用 Tiny-TTF：

```c
const lv_font_t *ui_font_mixed(uint8_t size)
{
    // 常用汉字使用内置字体
    static lv_font_t *builtin_cn = NULL;
    if (!builtin_cn) {
        // 使用 fonttools 创建子集
        builtin_cn = lv_binfont_create("builtin_cn.ttf");
    }
    return builtin_cn ? builtin_cn : ui_font_cn(size);
}
```

### 8.3 异步字体加载

避免字体加载阻塞 UI：

```c
void ui_font_init_async(void)
{
    // 在后台线程加载字体
    // ...
}
```

---

## 九、文件清单

完成中文适配需要以下文件：

```
项目目录/
├── lv_conf.h                              # LVGL 配置
├── main/
│   └── UI/
│       ├── fonts/
│       │   ├── NotoSansSC-Regular.ttf      # ⭐ 必需：中文字体
│       │   ├── fa-solid-900.ttf            # ⭐ 必需：图标字体
│       │   ├── fa-regular-400.ttf          # 图标字体
│       │   ├── ui_font.h                   # ⭐ 必需：字体管理头文件
│       │   └── ui_font.c                   # ⭐ 必需：字体管理实现
│       └── utils/
│           └── ui_i18n.c                   # 国际化（含字体切换）
└── CMakeLists.txt                          # 构建配置（含字体复制命令）
```

---

## 十、快速检查清单

- [ ] `lv_conf.h` 中 `LV_USE_TINY_TTF = 1`
- [ ] `lv_conf.h` 中 `LV_FONT_DEFAULT` 已设置
- [ ] 字体文件 `NotoSansSC-Regular.ttf` 在可执行文件目录
- [ ] `ui_font_init()` 在 `lv_init()` 之后调用
- [ ] 所有标签创建使用 `ui_i18n_font()` 而非直接用字体常量
- [ ] 构建日志无字体加载错误

---

**文档版本：** 1.0
**更新日期：** 2026-05-09
**适用版本：** LVGL v9.x
