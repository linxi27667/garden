#include "ui_page_garden.h"
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
    lv_obj_t *sw_veg;
    lv_obj_t *sw_fan;
    lv_obj_t *sw_auto;
    lv_obj_t *btn_hang;
    lv_obj_t *btn_collect;
    lv_obj_t *lbl_soil_val;
    lv_obj_t *lbl_light_val;
    lv_obj_t *lbl_rain;
    lv_obj_t *lbl_corridor_st;
    lv_obj_t *lbl_strip_st;
    lv_obj_t *lbl_veg_st;
    lv_obj_t *lbl_fan_st;
    lv_obj_t *lbl_servo_st;
    lv_obj_t *lbl_auto_st;
    bool deleted;
} garden_ctx_t;

static void on_sw_corridor(lv_event_t *e)
{
    garden_model_set_device("corridor_light",
        lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void on_sw_strip(lv_event_t *e)
{
    garden_model_set_device("strip_light",
        lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void on_sw_veg(lv_event_t *e)
{
    garden_model_set_device("veg_light",
        lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void on_sw_fan(lv_event_t *e)
{
    garden_model_set_device("water_pump",
        lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void on_sw_auto(lv_event_t *e)
{
    garden_model_set_auto(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED));
}

static void on_btn_hang(lv_event_t *e)
{
    (void)e;
    garden_model_set_servo(true);
}

static void on_btn_collect(lv_event_t *e)
{
    (void)e;
    garden_model_set_servo(false);
}

static void update_ui(garden_ctx_t *ctx)
{
    if (!ctx || ctx->deleted) return;
    const garden_model_t *st = garden_model_get();

    if (ctx->sw_corridor) {
        if (st->corridor_light) lv_obj_add_state(ctx->sw_corridor, LV_STATE_CHECKED);
        else lv_obj_clear_state(ctx->sw_corridor, LV_STATE_CHECKED);
    }
    if (ctx->lbl_corridor_st) {
        lv_label_set_text(ctx->lbl_corridor_st,
            st->corridor_light ? ui_i18n_get(UI_KEY_ON) : ui_i18n_get(UI_KEY_OFF));
        lv_obj_set_style_text_color(ctx->lbl_corridor_st,
            st->corridor_light ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    }

    if (ctx->sw_strip) {
        if (st->indoor_strip) lv_obj_add_state(ctx->sw_strip, LV_STATE_CHECKED);
        else lv_obj_clear_state(ctx->sw_strip, LV_STATE_CHECKED);
    }
    if (ctx->lbl_strip_st) {
        lv_label_set_text(ctx->lbl_strip_st,
            st->indoor_strip ? ui_i18n_get(UI_KEY_ON) : ui_i18n_get(UI_KEY_OFF));
        lv_obj_set_style_text_color(ctx->lbl_strip_st,
            st->indoor_strip ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    }

    if (ctx->sw_veg) {
        if (st->veg_light) lv_obj_add_state(ctx->sw_veg, LV_STATE_CHECKED);
        else lv_obj_clear_state(ctx->sw_veg, LV_STATE_CHECKED);
    }
    if (ctx->lbl_veg_st) {
        lv_label_set_text(ctx->lbl_veg_st,
            st->veg_light ? ui_i18n_get(UI_KEY_ON) : ui_i18n_get(UI_KEY_OFF));
        lv_obj_set_style_text_color(ctx->lbl_veg_st,
            st->veg_light ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    }

    if (ctx->sw_fan) {
        if (st->water_pump) lv_obj_add_state(ctx->sw_fan, LV_STATE_CHECKED);
        else lv_obj_clear_state(ctx->sw_fan, LV_STATE_CHECKED);
    }
    if (ctx->lbl_fan_st) {
        lv_label_set_text(ctx->lbl_fan_st,
            st->water_pump ? ui_i18n_get(UI_KEY_ON) : ui_i18n_get(UI_KEY_OFF));
        lv_obj_set_style_text_color(ctx->lbl_fan_st,
            st->water_pump ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    }

    if (ctx->lbl_servo_st) {
        lv_label_set_text(ctx->lbl_servo_st,
            st->servo_hang ? ui_i18n_get(UI_KEY_GARDEN_HUNG_OUT) : ui_i18n_get(UI_KEY_GARDEN_COLLECTED));
        lv_obj_set_style_text_color(ctx->lbl_servo_st,
            st->servo_hang ? UI_COLOR_GREEN : UI_COLOR_ORANGE, 0);
    }
    if (ctx->sw_auto) {
        if (st->auto_mode) lv_obj_add_state(ctx->sw_auto, LV_STATE_CHECKED);
        else lv_obj_clear_state(ctx->sw_auto, LV_STATE_CHECKED);
    }
    if (ctx->lbl_auto_st) {
        lv_label_set_text(ctx->lbl_auto_st,
            st->auto_mode ? ui_i18n_get(UI_KEY_GARDEN_AUTO_MODE) : ui_i18n_get(UI_KEY_GARDEN_MANUAL));
        lv_obj_set_style_text_color(ctx->lbl_auto_st,
            st->auto_mode ? UI_COLOR_GREEN : UI_COLOR_TEXT_SEC, 0);
    }

    if (ctx->lbl_soil_val) {
        char b[16];
        lv_snprintf(b, sizeof(b), "%d%%", (int)st->soil_moisture);
        lv_label_set_text(ctx->lbl_soil_val, b);
    }
    if (ctx->lbl_light_val) {
        char b[16];
        lv_snprintf(b, sizeof(b), "%d%%", (int)(st->light_sensor * 100 / 1023));
        lv_label_set_text(ctx->lbl_light_val, b);
    }
    if (ctx->lbl_rain) {
        lv_label_set_text(ctx->lbl_rain,
            st->rain_wet ? ui_i18n_get(UI_KEY_GARDEN_RAINING) : ui_i18n_get(UI_KEY_GARDEN_DRY));
        lv_obj_set_style_text_color(ctx->lbl_rain,
            st->rain_wet ? UI_COLOR_BLUE : UI_COLOR_GREEN, 0);
    }
}

static void on_model_updated(void *user_data)
{
    garden_ctx_t *ctx = (garden_ctx_t *)user_data;
    update_ui(ctx);
}

static void page_garden_delete(lv_event_t *e)
{
    garden_ctx_t *ctx = (garden_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;
    ctx->deleted = true;
    ui_event_unsubscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    lv_free(ctx);
}

static void section_hdr(lv_obj_t *parent, const char *text, lv_color_t dot_color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, LV_SIZE_CONTENT);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_dot(row, dot_color, 4);
    lv_obj_t *lbl = ui_create_label(row, text, ui_i18n_font(UI_FONT_14), UI_COLOR_TEXT);
    lv_obj_set_style_pad_left(lbl, 8, 0);
}

static lv_obj_t *device_card(lv_obj_t *parent, lv_color_t accent,
    const char *name, const char *init_st, lv_color_t init_st_color,
    lv_obj_t **out_st_lbl, lv_event_cb_t sw_cb)
{
    lv_obj_t *card = ui_create_card(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(card, 14, 0);

    ui_create_dot(card, accent, 10);

    lv_obj_t *info = lv_obj_create(card);
    lv_obj_remove_style_all(info);
    lv_obj_set_flex_grow(info, 1);
    lv_obj_set_layout(info, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(info, 3, 0);
    lv_obj_clear_flag(info, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_label(info, name, ui_i18n_font(UI_FONT_14), UI_COLOR_TEXT);
    *out_st_lbl = ui_create_label(info, init_st, ui_i18n_font(UI_FONT_12), init_st_color);

    lv_obj_t *sw = ui_create_switch(card, accent);
    lv_obj_add_event_cb(sw, sw_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return sw;
}

static lv_obj_t *sensor_tile(lv_obj_t *parent, lv_color_t accent,
    const char *name)
{
    lv_obj_t *card = ui_create_card(parent);
    lv_obj_set_flex_grow(card, 1);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(card, 6, 0);

    lv_obj_t *top = lv_obj_create(card);
    lv_obj_remove_style_all(top);
    lv_obj_set_width(top, lv_pct(100));
    lv_obj_set_height(top, LV_SIZE_CONTENT);
    lv_obj_set_layout(top, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(top, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(top, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_dot(top, accent, 8);
    ui_create_label(top, name, ui_i18n_font(UI_FONT_12), UI_COLOR_TEXT_SEC);

    return card;
}

lv_obj_t *ui_page_garden_create(lv_obj_t *parent)
{
    garden_ctx_t *ctx = lv_malloc(sizeof(garden_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t *page = ui_create_page(parent, page_garden_delete, ctx);

    section_hdr(page, ui_i18n_get(UI_KEY_GARDEN_LIGHTING), UI_COLOR_ORANGE);

    ctx->sw_corridor = device_card(page, UI_COLOR_ORANGE,
        ui_i18n_get(UI_KEY_GARDEN_CORRIDOR),
        ui_i18n_get(UI_KEY_OFF), UI_COLOR_TEXT_SEC,
        &ctx->lbl_corridor_st, on_sw_corridor);

    ctx->sw_strip = device_card(page, UI_COLOR_PURPLE,
        ui_i18n_get(UI_KEY_GARDEN_STRIP),
        ui_i18n_get(UI_KEY_OFF), UI_COLOR_TEXT_SEC,
        &ctx->lbl_strip_st, on_sw_strip);

    ctx->sw_veg = device_card(page, UI_COLOR_GREEN,
        ui_i18n_get(UI_KEY_GARDEN_VEG),
        ui_i18n_get(UI_KEY_OFF), UI_COLOR_TEXT_SEC,
        &ctx->lbl_veg_st, on_sw_veg);

    section_hdr(page, ui_i18n_get(UI_KEY_GARDEN_DEVICES), UI_COLOR_BLUE);

    ctx->sw_fan = device_card(page, UI_COLOR_BLUE,
        ui_i18n_get(UI_KEY_GARDEN_FAN),
        ui_i18n_get(UI_KEY_OFF), UI_COLOR_TEXT_SEC,
        &ctx->lbl_fan_st, on_sw_fan);

    lv_obj_t *servo_card = ui_create_card(page);
    lv_obj_set_width(servo_card, lv_pct(100));
    lv_obj_set_flex_flow(servo_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(servo_card, 12, 0);

    lv_obj_t *s_row = lv_obj_create(servo_card);
    lv_obj_remove_style_all(s_row);
    lv_obj_set_width(s_row, lv_pct(100));
    lv_obj_set_height(s_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(s_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(s_row, 10, 0);
    lv_obj_clear_flag(s_row, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_dot(s_row, UI_COLOR_ORANGE, 10);
    ui_create_label(s_row, ui_i18n_get(UI_KEY_GARDEN_SERVO),
        ui_i18n_font(UI_FONT_14), UI_COLOR_TEXT);
    ctx->lbl_servo_st = ui_create_label(s_row,
        ui_i18n_get(UI_KEY_GARDEN_HUNG_OUT),
        ui_i18n_font(UI_FONT_12), UI_COLOR_GREEN);

    lv_obj_t *s_btns = lv_obj_create(servo_card);
    lv_obj_remove_style_all(s_btns);
    lv_obj_set_width(s_btns, lv_pct(100));
    lv_obj_set_height(s_btns, LV_SIZE_CONTENT);
    lv_obj_set_layout(s_btns, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(s_btns, LV_FLEX_FLOW_ROW);
    lv_obj_set_style_pad_column(s_btns, 10, 0);
    lv_obj_clear_flag(s_btns, LV_OBJ_FLAG_SCROLLABLE);

    ctx->btn_hang = lv_btn_create(s_btns);
    lv_obj_set_flex_grow(ctx->btn_hang, 1);
    lv_obj_set_height(ctx->btn_hang, 44);
    lv_obj_set_style_bg_color(ctx->btn_hang, UI_COLOR_GREEN, 0);
    lv_obj_set_style_bg_opa(ctx->btn_hang, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->btn_hang, UI_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(ctx->btn_hang, 0, 0);
    lv_obj_add_event_cb(ctx->btn_hang, on_btn_hang, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bh = lv_label_create(ctx->btn_hang);
    lv_label_set_text(bh, ui_i18n_get(UI_KEY_GARDEN_HANG));
    lv_obj_set_style_text_font(bh, ui_i18n_font(UI_FONT_14), 0);
    lv_obj_set_style_text_color(bh, lv_color_white(), 0);
    lv_obj_center(bh);

    ctx->btn_collect = lv_btn_create(s_btns);
    lv_obj_set_flex_grow(ctx->btn_collect, 1);
    lv_obj_set_height(ctx->btn_collect, 44);
    lv_obj_set_style_bg_color(ctx->btn_collect, UI_COLOR_ORANGE, 0);
    lv_obj_set_style_bg_opa(ctx->btn_collect, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->btn_collect, UI_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(ctx->btn_collect, 0, 0);
    lv_obj_add_event_cb(ctx->btn_collect, on_btn_collect, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bc = lv_label_create(ctx->btn_collect);
    lv_label_set_text(bc, ui_i18n_get(UI_KEY_GARDEN_COLLECT));
    lv_obj_set_style_text_font(bc, ui_i18n_font(UI_FONT_14), 0);
    lv_obj_set_style_text_color(bc, lv_color_white(), 0);
    lv_obj_center(bc);

    section_hdr(page, ui_i18n_get(UI_KEY_GARDEN_SENSORS), UI_COLOR_BLUE);

    lv_obj_t *sensor_row = lv_obj_create(page);
    lv_obj_remove_style_all(sensor_row);
    lv_obj_set_width(sensor_row, lv_pct(100));
    lv_obj_set_height(sensor_row, LV_SIZE_CONTENT);
    lv_obj_set_layout(sensor_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sensor_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(sensor_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_column(sensor_row, 8, 0);
    lv_obj_clear_flag(sensor_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *sc1 = sensor_tile(sensor_row, UI_COLOR_ORANGE,
        ui_i18n_get(UI_KEY_GARDEN_SOIL));
    ctx->lbl_soil_val = ui_create_label(sc1, "--%", ui_i18n_font(UI_FONT_36), UI_COLOR_TEXT);

    lv_obj_t *sc2 = sensor_tile(sensor_row, UI_COLOR_BLUE,
        ui_i18n_get(UI_KEY_GARDEN_LIGHT));
    ctx->lbl_light_val = ui_create_label(sc2, "--%", ui_i18n_font(UI_FONT_36), UI_COLOR_TEXT);

    lv_obj_t *sc3 = sensor_tile(sensor_row, UI_COLOR_BLUE,
        ui_i18n_get(UI_KEY_GARDEN_RAIN));
    ctx->lbl_rain = ui_create_label(sc3, ui_i18n_get(UI_KEY_GARDEN_DRY),
        ui_i18n_font(UI_FONT_24), UI_COLOR_GREEN);

    section_hdr(page, ui_i18n_get(UI_KEY_GARDEN_AUTO), UI_COLOR_GREEN);

    device_card(page, UI_COLOR_GREEN, ui_i18n_get(UI_KEY_GARDEN_AUTO),
        ui_i18n_get(UI_KEY_GARDEN_AUTO_MODE), UI_COLOR_GREEN,
        &ctx->lbl_auto_st, on_sw_auto);

    ui_event_subscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    update_ui(ctx);

    printf("[GARDEN] Garden page created\n");
    return page;
}
