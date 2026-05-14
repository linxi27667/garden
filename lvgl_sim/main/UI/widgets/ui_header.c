#include "ui_header.h"
#include "../core/ui_styles.h"
#include "../core/ui_events.h"
#include "../services/ui_i18n.h"
#include "../services/ui_icons.h"
#include "../model/garden_model.h"
#include "../fonts/ui_font.h"
#include <time.h>
#include <string.h>

typedef struct {
    lv_obj_t *status_lbl;
    lv_obj_t *clock_lbl;
    lv_obj_t *status_dot;
    lv_obj_t *logo_icon;
    lv_obj_t *logo_text;
    lv_timer_t *clock_timer;
} header_ctx_t;

static void clock_tick(lv_timer_t *timer)
{
    header_ctx_t *ctx = (header_ctx_t *)lv_timer_get_user_data(timer);
    if (!ctx || !ctx->clock_lbl) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char buf[16];
    lv_snprintf(buf, sizeof(buf), "%02d:%02d", t->tm_hour, t->tm_min);
    lv_label_set_text(ctx->clock_lbl, buf);
}

static void on_model_updated(void *user_data)
{
    header_ctx_t *ctx = (header_ctx_t *)user_data;
    if (!ctx) return;
    const garden_model_t *m = garden_model_get();
    bool online = (m->wifi_state == WIFI_STATE_CONNECTED);

    if (ctx->status_lbl) {
        lv_label_set_text(ctx->status_lbl,
            ui_i18n_get(online ? UI_KEY_STATUS_ONLINE : UI_KEY_STATUS_OFFLINE));
    }
    if (ctx->status_dot) {
        lv_obj_set_style_bg_color(ctx->status_dot,
            online ? UI_COLOR_GREEN : UI_COLOR_RED, 0);
    }
}

static void header_delete_cb(lv_event_t *e)
{
    (void)e;
    lv_obj_t *header = lv_event_get_target(e);
    header_ctx_t *ctx = (header_ctx_t *)lv_obj_get_user_data(header);
    if (ctx) {
        if (ctx->clock_timer) {
            lv_timer_delete(ctx->clock_timer);
            ctx->clock_timer = NULL;
        }
        ui_event_unsubscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
        lv_free(ctx);
        lv_obj_set_user_data(header, NULL);
    }
}

lv_obj_t *ui_header_create(lv_obj_t *parent)
{
    header_ctx_t *ctx = lv_malloc(sizeof(header_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_user_data(header, ctx);
    lv_obj_set_size(header, lv_pct(100), UI_HEADER_HEIGHT);
    lv_obj_set_style_bg_color(header, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(header, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_pad_hor(header, 20, 0);
    lv_obj_set_style_pad_ver(header, 0, 0);
    lv_obj_set_layout(header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(header, header_delete_cb, LV_EVENT_DELETE, NULL);

    lv_obj_t *logo_area = lv_obj_create(header);
    lv_obj_remove_style_all(logo_area);
    lv_obj_set_style_bg_opa(logo_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(logo_area, 0, 0);
    lv_obj_set_style_pad_all(logo_area, 0, 0);
    lv_obj_set_layout(logo_area, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(logo_area, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(logo_area, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(logo_area, 10, 0);
    lv_obj_clear_flag(logo_area, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *logo_bg = lv_obj_create(logo_area);
    lv_obj_remove_style_all(logo_bg);
    lv_obj_set_size(logo_bg, 36, 36);
    lv_obj_set_style_bg_color(logo_bg, UI_COLOR_LOGO_BG, 0);
    lv_obj_set_style_bg_opa(logo_bg, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(logo_bg, 10, 0);
    lv_obj_set_style_border_width(logo_bg, 0, 0);
    lv_obj_set_layout(logo_bg, LV_LAYOUT_FLEX);
    lv_obj_set_flex_align(logo_bg, LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(logo_bg, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ctx->logo_icon = lv_label_create(logo_bg);
    lv_label_set_text(ctx->logo_icon, "\xEF\x80\x95");
    lv_obj_set_style_text_font(ctx->logo_icon, ui_font_icon(18), 0);
    lv_obj_set_style_text_color(ctx->logo_icon, lv_color_white(), 0);

    ctx->logo_text = ui_create_label(logo_area, ui_i18n_get(UI_KEY_BRAND),
        ui_i18n_font(UI_FONT_24), UI_COLOR_TEXT);

    lv_obj_t *right = lv_obj_create(header);
    lv_obj_remove_style_all(right);
    lv_obj_set_style_bg_opa(right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(right, 0, 0);
    lv_obj_set_style_pad_all(right, 0, 0);
    lv_obj_set_layout(right, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(right, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(right, LV_FLEX_ALIGN_END,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(right, 16, 0);
    lv_obj_clear_flag(right, LV_OBJ_FLAG_SCROLLABLE);

    ctx->clock_lbl = lv_label_create(right);
    lv_label_set_text(ctx->clock_lbl, "--:--");
    lv_obj_set_style_text_font(ctx->clock_lbl, ui_i18n_font(UI_FONT_24), 0);
    lv_obj_set_style_text_color(ctx->clock_lbl, UI_COLOR_TEXT, 0);

    lv_obj_t *status = lv_obj_create(right);
    lv_obj_remove_style_all(status);
    lv_obj_set_height(status, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(status, UI_COLOR_STATUS_BG, 0);
    lv_obj_set_style_bg_opa(status, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(status, 12, 0);
    lv_obj_set_style_pad_ver(status, 4, 0);
    lv_obj_set_style_pad_hor(status, 10, 0);
    lv_obj_set_style_border_width(status, 0, 0);
    lv_obj_set_layout(status, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(status, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(status, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(status, 6, 0);
    lv_obj_clear_flag(status, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    ctx->status_dot = lv_obj_create(status);
    lv_obj_remove_style_all(ctx->status_dot);
    lv_obj_set_size(ctx->status_dot, 8, 8);
    lv_obj_set_style_bg_color(ctx->status_dot, UI_COLOR_RED, 0);
    lv_obj_set_style_bg_opa(ctx->status_dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->status_dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(ctx->status_dot, 0, 0);
    lv_obj_clear_flag(ctx->status_dot, LV_OBJ_FLAG_SCROLLABLE);

    ctx->status_lbl = ui_create_label(status, ui_i18n_get(UI_KEY_STATUS_OFFLINE),
        ui_i18n_font(UI_FONT_SMALL), UI_COLOR_RED);

    ctx->clock_timer = lv_timer_create(clock_tick, 10000, ctx);
    clock_tick(ctx->clock_timer);

    ui_event_subscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);

    return header;
}

void ui_header_refresh_text(lv_obj_t *header)
{
    header_ctx_t *ctx = (header_ctx_t *)lv_obj_get_user_data(header);
    if (!ctx) return;
    const garden_model_t *m = garden_model_get();
    bool online = (m->wifi_state == WIFI_STATE_CONNECTED);
    if (ctx->status_lbl) {
        lv_label_set_text(ctx->status_lbl,
            ui_i18n_get(online ? UI_KEY_STATUS_ONLINE : UI_KEY_STATUS_OFFLINE));
    }
    if (ctx->logo_text) {
        lv_label_set_text(ctx->logo_text, ui_i18n_get(UI_KEY_BRAND));
    }
}
