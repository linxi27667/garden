#include "ui.h"
#include "core/ui_app.h"

void ui_init(void)
{
    ui_app_init();
}

void ui_switch_page(ui_page_id_t page)
{
    ui_app_switch_page(page);
}

void ui_rebuild_current(void)
{
    ui_app_rebuild_current();
}
