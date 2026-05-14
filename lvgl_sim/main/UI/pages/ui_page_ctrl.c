#include "ui_page_ctrl.h"
#include "../core/ui_styles.h"
#include "../core/ui_events.h"
#include "../services/ui_i18n.h"
#include "../services/ui_icons.h"
#include "../model/garden_model.h"
#include "../fonts/ui_font.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *sw_corridor;
    lv_obj_t *sw_strip;
    lv_obj_t *sw_water_pump;
    lv_obj_t *sw_mist_cooler;
    lv_obj_t *sw_auto;
    lv_obj_t *lbl_auto;
    bool deleted;
} ctrl_ctx_t;

static const char *DEV_CORRIDOR    = "corridor_light";
static const char *DEV_STRIP     = "strip_light";
static const char *DEV_WATER_PUMP = "water_pump";
static const char *DEV_MIST_COOLER = "mist_cooler";

static void sw_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    const char *dev = (const char *)lv_event_get_user_data(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    printf("[CTRL] %s -> %s\n", dev, on ? "ON" : "OFF");
    garden_model_set_device(dev, on);
}

static lv_obj_t *create_device_card(lv_obj_t *parent, const char *icon,
    const char *name, const char *device_key, lv_color_t accent,
    lv_obj_t **out_sw)
{
    lv_obj_t *card = ui_create_card(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = lv_obj_create(card);
    lv_obj_remove_style_all(left);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_style_pad_top(left, 8, 0);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(left, 12, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon_bg = lv_obj_create(left);
    lv_obj_remove_style_all(icon_bg);
    lv_obj_set_size(icon_bg, 40, 40);
    lv_obj_set_style_bg_color(icon_bg, accent, 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_10, 0);
    lv_obj_set_style_radius(icon_bg, 12, 0);
    lv_obj_set_layout(icon_bg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(icon_bg, LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon_lbl = lv_label_create(icon_bg);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_font(icon_lbl, ui_font_icon(18), 0);
    lv_obj_set_style_text_color(icon_lbl, accent, 0);

    ui_create_label(left, name, ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);

    lv_obj_t *sw = ui_create_switch(card, accent);
    lv_obj_add_event_cb(sw, sw_cb, LV_EVENT_VALUE_CHANGED,
        (void *)device_key);
    *out_sw = sw;

    return card;
}

static void on_scene(lv_event_t *e)
{
    int scene = (int)(intptr_t)lv_event_get_user_data(e);
    switch (scene) {
        case 0:
            garden_model_set_device(DEV_CORRIDOR, true);
            garden_model_set_device(DEV_STRIP, false);
            garden_model_set_device(DEV_WATER_PUMP, true);
            garden_model_set_device(DEV_MIST_COOLER, false);
            break;
        case 1:
            garden_model_set_device(DEV_CORRIDOR, true);
            garden_model_set_device(DEV_STRIP, true);
            garden_model_set_device(DEV_WATER_PUMP, false);
            garden_model_set_device(DEV_MIST_COOLER, false);
            break;
        case 2:
            garden_model_set_device(DEV_CORRIDOR, false);
            garden_model_set_device(DEV_STRIP, false);
            garden_model_set_device(DEV_WATER_PUMP, false);
            garden_model_set_device(DEV_MIST_COOLER, true);
            break;
    }
}

static void on_auto_toggle(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target(e);
    bool on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    garden_model_set_auto(on);
}

static void sync_switch(lv_obj_t *sw, bool on)
{
    if (!sw) return;
    if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
    else lv_obj_clear_state(sw, LV_STATE_CHECKED);
}

static void on_model_updated(void *user_data)
{
    ctrl_ctx_t *ctx = (ctrl_ctx_t *)user_data;
    if (!ctx || ctx->deleted) return;
    const garden_model_t *m = garden_model_get();

    sync_switch(ctx->sw_corridor, m->corridor_light);
    sync_switch(ctx->sw_strip, m->indoor_strip);
    sync_switch(ctx->sw_water_pump, m->water_pump);
    sync_switch(ctx->sw_mist_cooler, m->mist_cooler);
    sync_switch(ctx->sw_auto, m->auto_mode);

    if (ctx->lbl_auto) {
        lv_label_set_text(ctx->lbl_auto,
            ui_i18n_get(m->auto_mode ? UI_KEY_AUTO_MODE : UI_KEY_MANUAL));
    }
}

static void page_ctrl_delete(lv_event_t *e)
{
    ctrl_ctx_t *ctx = (ctrl_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;
    ctx->deleted = true;
    ui_event_unsubscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    lv_free(ctx);
}

lv_obj_t *ui_page_ctrl_create(lv_obj_t *parent)
{
    ctrl_ctx_t *ctx = lv_malloc(sizeof(ctrl_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t *page = ui_create_page(parent, page_ctrl_delete, ctx);

    ui_create_title_row(page, ICON_SETTINGS,
        ui_i18n_get(UI_KEY_CTRL_TITLE));

    lv_obj_t *auto_row = ui_create_card(page);
    lv_obj_set_width(auto_row, lv_pct(100));
    lv_obj_set_flex_flow(auto_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(auto_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    ctx->lbl_auto = ui_create_label(auto_row,
        ui_i18n_get(UI_KEY_AUTO_MODE), ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);
    ctx->sw_auto = ui_create_switch(auto_row, UI_COLOR_GREEN);
    lv_obj_add_event_cb(ctx->sw_auto, on_auto_toggle,
        LV_EVENT_VALUE_CHANGED, NULL);

    lv_obj_t *scene_lbl = ui_create_label(page,
        ui_i18n_get(UI_KEY_CTRL_SCENE), ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT_SEC);

    lv_obj_t *scene_row = lv_obj_create(page);
    lv_obj_remove_style_all(scene_row);
    lv_obj_set_style_pad_all(scene_row, 0, 0);
    lv_obj_set_layout(scene_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scene_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(scene_row, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(scene_row, 10, 0);
    lv_obj_clear_flag(scene_row, LV_OBJ_FLAG_SCROLLABLE);

    const ui_i18n_key_t scene_keys[] = {
        UI_KEY_CTRL_MORNING, UI_KEY_CTRL_NIGHT, UI_KEY_CTRL_AWAY
    };
    const lv_color_t scene_colors[] = {
        UI_COLOR_ORANGE, UI_COLOR_PURPLE, UI_COLOR_BLUE
    };
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(scene_row);
        lv_obj_set_style_bg_color(btn, scene_colors[i], 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(btn, UI_BTN_RADIUS, 0);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_ver(btn, 8, 0);
        lv_obj_set_style_pad_hor(btn, 16, 0);
        lv_obj_add_event_cb(btn, on_scene, LV_EVENT_CLICKED,
            (void *)(intptr_t)i);

        ui_create_label(btn,
            ui_i18n_get(scene_keys[i]),
            ui_i18n_font(UI_FONT_SMALL), lv_color_white());
    }

    ui_create_label(page, ui_i18n_get(UI_KEY_DEVICES),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT_SEC);

    create_device_card(page, ICON_SUN,
        ui_i18n_get(UI_KEY_DEVICE_COURTYARD),
        DEV_CORRIDOR, UI_COLOR_ORANGE, &ctx->sw_corridor);

    create_device_card(page, ICON_BOLT,
        ui_i18n_get(UI_KEY_DEVICE_STRIP),
        DEV_STRIP, UI_COLOR_BLUE, &ctx->sw_strip);

    create_device_card(page, ICON_DROP,
        ui_i18n_get(UI_KEY_DEVICE_GARDEN),
        DEV_WATER_PUMP, UI_COLOR_GREEN, &ctx->sw_water_pump);

    create_device_card(page, ICON_FOG,
        ui_i18n_get(UI_KEY_DEVICE_FAN),
        DEV_MIST_COOLER, UI_COLOR_RED, &ctx->sw_mist_cooler);

    ui_event_subscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    on_model_updated(ctx);

    printf("[CTRL] Control page created\n");
    return page;
}
