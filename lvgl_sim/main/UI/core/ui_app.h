#ifndef UI_APP_H
#define UI_APP_H

#include "lvgl/lvgl.h"
#include "../utils/ui_theme.h"

typedef enum {
    UI_PAGE_DATA = 0,
    UI_PAGE_CTRL = 1,
    UI_PAGE_SET  = 2,
    UI_PAGE_COUNT = 3,
} ui_page_id_t;

void ui_app_init(void);
void ui_app_switch_page(ui_page_id_t page);
void ui_app_rebuild_current(void);
void ui_app_poll(void);

#endif
