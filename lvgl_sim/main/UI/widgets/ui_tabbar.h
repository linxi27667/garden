#ifndef UI_TABBAR_H
#define UI_TABBAR_H

#include "lvgl/lvgl.h"

typedef void (*ui_tab_cb_t)(void *user_data);

lv_obj_t *ui_tabbar_create(lv_obj_t *parent, ui_tab_cb_t cb);
void ui_tabbar_set_active(lv_obj_t *tabbar, int active);
void ui_tabbar_refresh_text(lv_obj_t *tabbar);

#endif
