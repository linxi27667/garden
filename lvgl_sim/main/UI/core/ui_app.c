#include "ui_app.h"
#include "ui_events.h"
#include "ui_styles.h"
#include "../fonts/ui_font.h"
#include "../services/ui_i18n.h"
#include "../model/garden_model.h"
#include "../pages/ui_page_data.h"
#include "../pages/ui_page_ctrl.h"
#include "../pages/ui_page_set.h"
#include "../widgets/ui_header.h"
#include "../widgets/ui_tabbar.h"
#include "lvgl/lvgl.h"
#include <string.h>
#include <stdio.h>

typedef struct {
    lv_obj_t *content_parent;
    lv_obj_t *tabbar;
    lv_obj_t *header;
    ui_page_id_t current_page;
    volatile int switch_pending;
    ui_page_id_t target_page;
} app_ctx_t;

static app_ctx_t s_app_ctx;

static void reset_all_indev(void)
{
    lv_indev_t *indev = lv_indev_get_next(NULL);
    while (indev) {
        lv_indev_reset(indev, NULL);
        indev = lv_indev_get_next(indev);
    }
}

static void do_switch_page(void)
{
    ui_page_id_t page = s_app_ctx.target_page;
    s_app_ctx.switch_pending = 0;

    if (s_app_ctx.content_parent) {
        reset_all_indev();
        lv_obj_clean(s_app_ctx.content_parent);
        lv_obj_scroll_to(s_app_ctx.content_parent, 0, 0, LV_ANIM_OFF);

        switch (page) {
            case UI_PAGE_DATA: ui_page_data_create(s_app_ctx.content_parent); break;
            case UI_PAGE_CTRL: ui_page_ctrl_create(s_app_ctx.content_parent); break;
            case UI_PAGE_SET:  ui_page_set_create(s_app_ctx.content_parent); break;
            default: break;
        }
    }
    if (s_app_ctx.tabbar) {
        ui_tabbar_set_active(s_app_ctx.tabbar, page);
    }
    s_app_ctx.current_page = page;
    ui_event_publish(UI_EVENT_PAGE_SWITCHED);
}

static void on_tab(void *user_data)
{
    ui_page_id_t page = (ui_page_id_t)(intptr_t)user_data;
    if (s_app_ctx.switch_pending && s_app_ctx.target_page == page) return;
    s_app_ctx.switch_pending = 1;
    s_app_ctx.target_page = page;
}

static void on_lang_changed(void *user_data)
{
    (void)user_data;
    if (s_app_ctx.header) {
        ui_header_refresh_text(s_app_ctx.header);
    }
    if (s_app_ctx.tabbar) {
        ui_tabbar_refresh_text(s_app_ctx.tabbar);
    }
    s_app_ctx.switch_pending = 1;
    s_app_ctx.target_page = s_app_ctx.current_page;
}

static void create_content_area(lv_obj_t *scr)
{
    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_width(content, lv_pct(100));
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_color(content, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    s_app_ctx.content_parent = content;
}

void ui_app_switch_page(ui_page_id_t page)
{
    if ((int)page < 0 || (int)page >= UI_PAGE_COUNT) return;
    if (s_app_ctx.switch_pending && s_app_ctx.target_page == page) return;
    s_app_ctx.switch_pending = 1;
    s_app_ctx.target_page = page;
}

void ui_app_rebuild_current(void)
{
    s_app_ctx.switch_pending = 1;
    s_app_ctx.target_page = s_app_ctx.current_page;
}

void ui_app_poll(void)
{
    if (s_app_ctx.switch_pending) {
        do_switch_page();
    }
}

void ui_app_init(void)
{
    memset(&s_app_ctx, 0, sizeof(s_app_ctx));
    s_app_ctx.current_page = UI_PAGE_DATA;

    ui_events_init();
    ui_theme_init();
    ui_styles_init();
    ui_font_init();
    ui_i18n_init();
    garden_model_init();
    garden_model_start();

    ui_event_subscribe(UI_EVENT_LANG_CHANGED, on_lang_changed, NULL);

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_layout(scr, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_gap(scr, 0, 0);

    s_app_ctx.header = ui_header_create(scr);

    create_content_area(scr);

    s_app_ctx.tabbar = ui_tabbar_create(scr, on_tab);

    if (s_app_ctx.content_parent) {
        ui_page_data_create(s_app_ctx.content_parent);
    }
    if (s_app_ctx.tabbar) {
        ui_tabbar_set_active(s_app_ctx.tabbar, UI_PAGE_DATA);
    }

    printf("[APP] UI initialized: 1024x600, 3 pages (DATA/CTRL/SET)\n");
}
