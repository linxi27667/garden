#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "lvgl/lvgl.h"

lv_obj_t *ui_header_create(lv_obj_t *parent);
void ui_header_refresh_text(lv_obj_t *header);

#endif
