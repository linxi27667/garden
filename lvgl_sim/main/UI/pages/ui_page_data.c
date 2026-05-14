#include "ui_page_data.h"
#include "../core/ui_styles.h"
#include "../core/ui_events.h"
#include "../services/ui_i18n.h"
#include "../services/ui_icons.h"
#include "../model/garden_model.h"
#include "../fonts/ui_font.h"
#include <stdio.h>
#include <string.h>

#define LIGHT_SENSOR_MAX  1023
#define SMOKE_SENSOR_MAX  100
#define RAIN_PERCENT_MAX  100

#define LIGHT_DIM_THRESHOLD  300
#define SMOKE_ALERT_THRESHOLD 60
#define RAIN_WET_THRESHOLD   500

typedef struct {
    lv_obj_t *lbl_light;
    lv_obj_t *lbl_smoke;
    lv_obj_t *lbl_rain;
    lv_obj_t *icon_bg_light;
    lv_obj_t *icon_bg_smoke;
    lv_obj_t *icon_bg_rain;
    lv_obj_t *bar_light;
    lv_obj_t *bar_smoke;
    lv_obj_t *bar_rain;
    bool deleted;
} data_ctx_t;

static void update_data(data_ctx_t *ctx)
{
    if (!ctx || ctx->deleted) return;

    const garden_model_t *m = garden_model_get();
    char buf[32];

    if (ctx->lbl_light) {
        int pct = (int)(m->light_sensor * 100 / LIGHT_SENSOR_MAX);
        lv_snprintf(buf, sizeof(buf), "%d%%", pct);
        lv_label_set_text(ctx->lbl_light, buf);
    }
    if (ctx->lbl_smoke) {
        lv_snprintf(buf, sizeof(buf), "%d %s", (int)m->smoke_sensor,
            ui_i18n_get(UI_KEY_PPM));
        lv_label_set_text(ctx->lbl_smoke, buf);
    }
    if (ctx->lbl_rain) {
        int pct = (int)(m->rain_sensor * RAIN_PERCENT_MAX / LIGHT_SENSOR_MAX);
        lv_snprintf(buf, sizeof(buf), "%d%s", pct,
            ui_i18n_get(UI_KEY_PERCENT));
        lv_label_set_text(ctx->lbl_rain, buf);
    }

    if (ctx->bar_light) {
        lv_bar_set_value(ctx->bar_light, m->light_sensor, LV_ANIM_ON);
    }
    if (ctx->bar_smoke) {
        lv_bar_set_value(ctx->bar_smoke, m->smoke_sensor, LV_ANIM_ON);
    }
    if (ctx->bar_rain) {
        lv_bar_set_value(ctx->bar_rain,
            m->rain_sensor * RAIN_PERCENT_MAX / LIGHT_SENSOR_MAX, LV_ANIM_ON);
    }

    if (ctx->icon_bg_light) {
        lv_color_t c = m->light_sensor < LIGHT_DIM_THRESHOLD ? UI_COLOR_ORANGE : UI_COLOR_ACCENT;
        lv_obj_set_style_bg_color(ctx->icon_bg_light, c, 0);
    }
    if (ctx->icon_bg_smoke) {
        lv_color_t c = m->smoke_sensor > SMOKE_ALERT_THRESHOLD ? UI_COLOR_RED : UI_COLOR_GREEN;
        lv_obj_set_style_bg_color(ctx->icon_bg_smoke, c, 0);
    }
    if (ctx->icon_bg_rain) {
        lv_color_t c = m->rain_sensor > RAIN_WET_THRESHOLD ? UI_COLOR_BLUE : UI_COLOR_ACCENT;
        lv_obj_set_style_bg_color(ctx->icon_bg_rain, c, 0);
    }
}

static void on_model_updated(void *user_data)
{
    data_ctx_t *ctx = (data_ctx_t *)user_data;
    update_data(ctx);
}

static void page_data_delete(lv_event_t *e)
{
    data_ctx_t *ctx = (data_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;
    ctx->deleted = true;
    ui_event_unsubscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    lv_free(ctx);
}

static lv_obj_t *create_sensor_card(lv_obj_t *parent, const char *icon,
    const char *name, lv_color_t accent, int max_val,
    lv_obj_t **out_val_lbl, lv_obj_t **out_bar, lv_obj_t **out_icon_bg)
{
    lv_obj_t *card = ui_create_card(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *left = lv_obj_create(card);
    lv_obj_remove_style_all(left);
    lv_obj_set_style_pad_all(left, 0, 0);
    lv_obj_set_layout(left, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(left, 12, 0);
    lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon_bg = lv_obj_create(left);
    lv_obj_remove_style_all(icon_bg);
    lv_obj_set_size(icon_bg, 44, 44);
    lv_obj_set_style_bg_color(icon_bg, accent, 0);
    lv_obj_set_style_bg_opa(icon_bg, LV_OPA_10, 0);
    lv_obj_set_style_radius(icon_bg, 14, 0);
    lv_obj_set_layout(icon_bg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(icon_bg, LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(icon_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *icon_lbl = lv_label_create(icon_bg);
    lv_label_set_text(icon_lbl, icon);
    lv_obj_set_style_text_font(icon_lbl, ui_font_icon(20), 0);
    lv_obj_set_style_text_color(icon_lbl, accent, 0);

    if (out_icon_bg) *out_icon_bg = icon_bg;

    lv_obj_t *name_col = lv_obj_create(left);
    lv_obj_remove_style_all(name_col);
    lv_obj_set_style_pad_all(name_col, 0, 0);
    lv_obj_set_layout(name_col, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(name_col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(name_col, 2, 0);
    lv_obj_clear_flag(name_col, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_label(name_col, name, ui_i18n_font(UI_FONT_SMALL), UI_COLOR_TEXT_SEC);

    lv_obj_t *val_lbl = ui_create_label(name_col, "--",
        ui_i18n_font(UI_FONT_VALUE), UI_COLOR_TEXT);
    *out_val_lbl = val_lbl;

    lv_obj_t *bar = lv_bar_create(card);
    lv_obj_set_size(bar, 80, 8);
    lv_bar_set_range(bar, 0, max_val);
    lv_bar_set_value(bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, UI_COLOR_BORDER, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(bar, 4, 0);
    lv_obj_set_style_bg_color(bar, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    *out_bar = bar;

    return card;
}

lv_obj_t *ui_page_data_create(lv_obj_t *parent)
{
    data_ctx_t *ctx = lv_malloc(sizeof(data_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t *page = ui_create_page(parent, page_data_delete, ctx);

    ui_create_title_row(page, ICON_CHART,
        ui_i18n_get(UI_KEY_DATA_REALTIME));

    ui_create_label(page, ui_i18n_get(UI_KEY_SENSORS),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT_SEC);

    create_sensor_card(page, ICON_SUN,
        ui_i18n_get(UI_KEY_SENSOR_LIGHT), UI_COLOR_ORANGE, LIGHT_SENSOR_MAX,
        &ctx->lbl_light, &ctx->bar_light, &ctx->icon_bg_light);

    create_sensor_card(page, ICON_AIR,
        ui_i18n_get(UI_KEY_SENSOR_SMOKE), UI_COLOR_GREEN, SMOKE_SENSOR_MAX,
        &ctx->lbl_smoke, &ctx->bar_smoke, &ctx->icon_bg_smoke);

    create_sensor_card(page, ICON_WATER,
        ui_i18n_get(UI_KEY_SENSOR_RAIN), UI_COLOR_BLUE, RAIN_PERCENT_MAX,
        &ctx->lbl_rain, &ctx->bar_rain, &ctx->icon_bg_rain);

    ui_event_subscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    update_data(ctx);

    printf("[DATA] Data page created\n");
    return page;
}
