#ifndef UI_FONT_H
#define UI_FONT_H

#include "lvgl/lvgl.h"

#define UI_FONT_12  &lv_font_montserrat_12
#define UI_FONT_14  &lv_font_montserrat_14
#define UI_FONT_16  &lv_font_montserrat_16
#define UI_FONT_20  &lv_font_montserrat_20
#define UI_FONT_24  &lv_font_montserrat_24
#define UI_FONT_36  &lv_font_montserrat_36
#define UI_FONT_48  &lv_font_montserrat_48

#define UI_FONT_TITLE  UI_FONT_20
#define UI_FONT_LABEL  UI_FONT_14
#define UI_FONT_SMALL  UI_FONT_12
#define UI_FONT_VALUE  UI_FONT_36

void ui_font_init(void);
const lv_font_t *ui_font_cn(uint8_t size);
const lv_font_t *ui_font_icon(uint8_t size);

#endif
