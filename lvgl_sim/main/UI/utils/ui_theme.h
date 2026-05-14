#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl/lvgl.h"

#define UI_COLOR_BG           lv_color_hex(0xF2F2F7)
#define UI_COLOR_CARD         lv_color_hex(0xFFFFFF)
#define UI_COLOR_ACCENT       lv_color_hex(0x007AFF)
#define UI_COLOR_TEXT         lv_color_hex(0x1C1C1E)
#define UI_COLOR_TEXT_SEC     lv_color_hex(0x8E8E93)
#define UI_COLOR_GREEN        lv_color_hex(0x34C759)
#define UI_COLOR_ORANGE       lv_color_hex(0xFF9500)
#define UI_COLOR_RED          lv_color_hex(0xFF3B30)
#define UI_COLOR_PURPLE       lv_color_hex(0xAF52DE)
#define UI_COLOR_BLUE         lv_color_hex(0x5AC8FA)
#define UI_COLOR_BORDER       lv_color_hex(0xE5E5EA)
#define UI_COLOR_INPUT_BG     lv_color_hex(0xF2F2F7)

#define UI_COLOR_SUCCESS      UI_COLOR_GREEN
#define UI_COLOR_WARNING      UI_COLOR_ORANGE
#define UI_COLOR_ERROR        UI_COLOR_RED

#define UI_COLOR_LOGO_BG      lv_color_hex(0x2E7D32)
#define UI_COLOR_STATUS_BG    lv_color_hex(0xE8F5E9)
#define UI_COLOR_TAB_PRESSED  lv_color_hex(0xE3F2FD)

#define UI_CONTENT_PAD      16
#define UI_CARD_RADIUS      16
#define UI_BTN_RADIUS       12
#define UI_INPUT_RADIUS     10
#define UI_HEADER_HEIGHT    56
#define UI_TAB_HEIGHT       60

void ui_theme_init(void);

#endif
