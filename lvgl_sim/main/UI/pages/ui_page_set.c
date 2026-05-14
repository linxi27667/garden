#include "ui_page_set.h"
#include "../core/ui_styles.h"
#include "../core/ui_events.h"
#include "../services/ui_i18n.h"
#include "../services/ui_icons.h"
#include "../model/garden_model.h"
#include "../fonts/ui_font.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    lv_obj_t *lbl_lang_value;
    lv_obj_t *lbl_wifi_status;
    lv_obj_t *wifi_list;
    lv_obj_t *btn_scan_obj;
    lv_obj_t *btn_disc_obj;
    lv_obj_t *pwd_card;
    lv_obj_t *lbl_selected_ssid;
    lv_obj_t *pwd_ta;
    lv_obj_t *btn_connect_obj;
    lv_obj_t *lbl_connect;
    char selected_ssid[WIFI_SSID_MAX];
    bool deleted;
} set_ctx_t;

static void on_lang_toggle(lv_event_t *e)
{
    ui_lang_t cur = ui_i18n_get_lang();
    ui_i18n_set_lang(cur == UI_LANG_ZH ? UI_LANG_EN : UI_LANG_ZH);
}

static void on_reset(lv_event_t *e)
{
    (void)e;
    garden_model_init();
    printf("[SET] Reset confirmed - model reinitialized\n");
}

static void on_wifi_scan(lv_event_t *e)
{
    set_ctx_t *ctx = (set_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;

    lv_obj_add_state(ctx->btn_scan_obj, LV_STATE_DISABLED);
    garden_model_wifi_scan();
}

static void on_wifi_ap_click(lv_event_t *e)
{
    set_ctx_t *ctx = (set_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;

    int idx = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    const garden_model_t *m = garden_model_get();
    if (idx < 0 || idx >= m->wifi_ap_count) return;

    strncpy(ctx->selected_ssid, m->wifi_aps[idx].ssid, WIFI_SSID_MAX - 1);
    ctx->selected_ssid[WIFI_SSID_MAX - 1] = '\0';

    lv_obj_clear_flag(ctx->pwd_card, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->lbl_selected_ssid, ctx->selected_ssid);
    lv_textarea_set_text(ctx->pwd_ta, "");

    printf("[SET] Selected WiFi: %s\n", ctx->selected_ssid);
}

static void on_wifi_connect(lv_event_t *e)
{
    set_ctx_t *ctx = (set_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;

    const char *pwd = ctx->pwd_ta ? lv_textarea_get_text(ctx->pwd_ta) : "";
    lv_label_set_text(ctx->lbl_connect, ui_i18n_get(UI_KEY_SET_WIFI_SCANNING));
    lv_obj_add_state(ctx->btn_connect_obj, LV_STATE_DISABLED);
    garden_model_wifi_connect(ctx->selected_ssid, pwd);
}

static void on_wifi_disconnect(lv_event_t *e)
{
    set_ctx_t *ctx = (set_ctx_t *)lv_event_get_user_data(e);
    if (!ctx) return;
    garden_model_wifi_disconnect();
}

static void rebuild_wifi_list(set_ctx_t *ctx)
{
    if (!ctx || !ctx->wifi_list) return;
    lv_obj_clean(ctx->wifi_list);

    const garden_model_t *m = garden_model_get();

    if (m->wifi_ap_count == 0) {
        lv_obj_t *hint = ui_create_label(ctx->wifi_list,
            ui_i18n_get(UI_KEY_SET_WIFI_NO_AP),
            ui_i18n_font(UI_FONT_SMALL), UI_COLOR_TEXT_SEC);
        lv_obj_set_width(hint, lv_pct(100));
        lv_label_set_long_mode(hint, LV_LABEL_LONG_WRAP);
        return;
    }

    for (int i = 0; i < m->wifi_ap_count; i++) {
        lv_obj_t *row = lv_obj_create(ctx->wifi_list);
        lv_obj_remove_style_all(row);
        lv_obj_set_width(row, lv_pct(100));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(row, UI_COLOR_CARD, 0);
        lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(row, 12, 0);
        lv_obj_set_style_border_width(row, 1, 0);
        lv_obj_set_style_border_color(row, UI_COLOR_BORDER, 0);
        lv_obj_set_style_border_opa(row, LV_OPA_COVER, 0);
        lv_obj_set_style_pad_ver(row, 12, 0);
        lv_obj_set_style_pad_hor(row, 14, 0);
        lv_obj_set_layout(row, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(row, (void *)(intptr_t)i);
        lv_obj_add_event_cb(row, on_wifi_ap_click, LV_EVENT_CLICKED, ctx);

        lv_obj_t *left = lv_obj_create(row);
        lv_obj_remove_style_all(left);
        lv_obj_set_style_pad_all(left, 0, 0);
        lv_obj_set_layout(left, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(left, LV_FLEX_ALIGN_START,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_column(left, 10, 0);
        lv_obj_clear_flag(left, LV_OBJ_FLAG_SCROLLABLE);

        ui_create_icon(left, ICON_WIFI, ui_font_icon(16), UI_COLOR_ACCENT);
        ui_create_label(left, m->wifi_aps[i].ssid,
            ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);

        char sig[16];
        lv_snprintf(sig, sizeof(sig), "%d dBm", (int)m->wifi_aps[i].rssi);
        ui_create_label(row, sig, UI_FONT_SMALL, UI_COLOR_TEXT_SEC);
    }
}

static void on_model_updated(void *user_data)
{
    set_ctx_t *ctx = (set_ctx_t *)user_data;
    if (!ctx || ctx->deleted) return;

    const garden_model_t *m = garden_model_get();

    if (ctx->lbl_wifi_status) {
        switch (m->wifi_state) {
            case WIFI_STATE_CONNECTED: {
                char buf[64];
                lv_snprintf(buf, sizeof(buf), "%s: %s",
                    ui_i18n_get(UI_KEY_SET_WIFI_CONNECTED), m->wifi_ssid);
                lv_label_set_text(ctx->lbl_wifi_status, buf);
                lv_obj_set_style_text_color(ctx->lbl_wifi_status, UI_COLOR_GREEN, 0);
                break;
            }
            case WIFI_STATE_CONNECTING:
                lv_label_set_text(ctx->lbl_wifi_status,
                    ui_i18n_get(UI_KEY_SET_WIFI_SCANNING));
                lv_obj_set_style_text_color(ctx->lbl_wifi_status, UI_COLOR_ORANGE, 0);
                break;
            case WIFI_STATE_FAILED:
                lv_label_set_text(ctx->lbl_wifi_status,
                    ui_i18n_get(UI_KEY_SET_WIFI_CONNECT_FAILED));
                lv_obj_set_style_text_color(ctx->lbl_wifi_status, UI_COLOR_RED, 0);
                break;
            default:
                lv_label_set_text(ctx->lbl_wifi_status, "");
                break;
        }
    }

    if (ctx->btn_scan_obj) {
        lv_obj_clear_state(ctx->btn_scan_obj, LV_STATE_DISABLED);
    }

    if (ctx->btn_disc_obj) {
        if (m->wifi_state == WIFI_STATE_CONNECTED) {
            lv_obj_clear_flag(ctx->btn_disc_obj, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(ctx->btn_disc_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }

    if (ctx->btn_connect_obj) {
        lv_obj_clear_state(ctx->btn_connect_obj, LV_STATE_DISABLED);
        if (ctx->lbl_connect) {
            lv_label_set_text(ctx->lbl_connect,
                ui_i18n_get(UI_KEY_SET_WIFI_CONNECT));
        }
    }

    if (m->wifi_state == WIFI_STATE_CONNECTED && ctx->pwd_card) {
        lv_obj_add_flag(ctx->pwd_card, LV_OBJ_FLAG_HIDDEN);
    }

    rebuild_wifi_list(ctx);
}

static void on_lang_changed_event(void *user_data)
{
    set_ctx_t *ctx = (set_ctx_t *)user_data;
    if (!ctx || ctx->deleted) return;
    if (ctx->lbl_lang_value) {
        lv_label_set_text(ctx->lbl_lang_value,
            ui_i18n_get(UI_KEY_SET_LANG_CURRENT));
    }
}

static void page_set_delete(lv_event_t *e)
{
    set_ctx_t *ctx = (set_ctx_t *)lv_event_get_user_data(e);
    if (ctx) {
        ctx->deleted = true;
        ui_event_unsubscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
        ui_event_unsubscribe(UI_EVENT_LANG_CHANGED, on_lang_changed_event, ctx);
        lv_free(ctx);
    }
}

static lv_obj_t *create_row_card(lv_obj_t *parent, bool clickable)
{
    lv_obj_t *card = ui_create_card(parent);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    if (clickable) {
        lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    }
    return card;
}

static lv_obj_t *create_icon_label_row(lv_obj_t *parent, const char *icon,
    const char *text, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, 8, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_icon(row, icon, ui_font_icon(16), color);
    ui_create_label(row, text, font, color);
    return row;
}

static lv_obj_t *create_btn(lv_obj_t *parent, const char *text,
    lv_color_t bg, lv_event_cb_t cb, void *user_data)
{
    lv_obj_t *btn = lv_btn_create(parent);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, UI_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_set_style_pad_ver(btn, 10, 0);
    lv_obj_set_style_pad_hor(btn, 24, 0);
    if (cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    ui_create_label(btn, text, ui_i18n_font(UI_FONT_SMALL), lv_color_white());
    return btn;
}

lv_obj_t *ui_page_set_create(lv_obj_t *parent)
{
    set_ctx_t *ctx = lv_malloc(sizeof(set_ctx_t));
    memset(ctx, 0, sizeof(*ctx));

    lv_obj_t *page = ui_create_page(parent, page_set_delete, ctx);

    ui_create_title_row(page, ICON_SETTINGS,
        ui_i18n_get(UI_KEY_SET_TITLE));

    lv_obj_t *lang_card = create_row_card(page, true);
    lv_obj_add_event_cb(lang_card, on_lang_toggle, LV_EVENT_CLICKED, NULL);
    create_icon_label_row(lang_card, ICON_LANGUAGE,
        ui_i18n_get(UI_KEY_SET_LANGUAGE),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);
    ctx->lbl_lang_value = ui_create_label(lang_card,
        ui_i18n_get(UI_KEY_SET_LANG_CURRENT),
        ui_i18n_font(UI_FONT_SMALL), UI_COLOR_TEXT_SEC);

    lv_obj_t *wifi_header = lv_obj_create(page);
    lv_obj_remove_style_all(wifi_header);
    lv_obj_set_style_pad_all(wifi_header, 0, 0);
    lv_obj_set_width(wifi_header, lv_pct(100));
    lv_obj_set_layout(wifi_header, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wifi_header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(wifi_header, LV_FLEX_ALIGN_SPACE_BETWEEN,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(wifi_header, LV_OBJ_FLAG_SCROLLABLE);

    create_icon_label_row(wifi_header, ICON_WIFI,
        ui_i18n_get(UI_KEY_SET_WIFI),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);

    ctx->lbl_wifi_status = ui_create_label(wifi_header, "",
        ui_i18n_font(UI_FONT_SMALL), UI_COLOR_GREEN);

    lv_obj_t *btn_row = lv_obj_create(page);
    lv_obj_remove_style_all(btn_row);
    lv_obj_set_style_pad_all(btn_row, 0, 0);
    lv_obj_set_layout(btn_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btn_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_row, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(btn_row, 10, 0);
    lv_obj_clear_flag(btn_row, LV_OBJ_FLAG_SCROLLABLE);

    ctx->btn_scan_obj = create_btn(btn_row,
        ui_i18n_get(UI_KEY_SET_WIFI_SCAN),
        UI_COLOR_ACCENT, on_wifi_scan, ctx);

    ctx->btn_disc_obj = create_btn(btn_row,
        ui_i18n_get(UI_KEY_SET_WIFI_DISCONNECT),
        UI_COLOR_RED, on_wifi_disconnect, ctx);
    lv_obj_add_flag(ctx->btn_disc_obj, LV_OBJ_FLAG_HIDDEN);

    ctx->wifi_list = lv_obj_create(page);
    lv_obj_remove_style_all(ctx->wifi_list);
    lv_obj_set_width(ctx->wifi_list, lv_pct(100));
    lv_obj_set_height(ctx->wifi_list, LV_SIZE_CONTENT);
    lv_obj_set_style_max_height(ctx->wifi_list, 240, 0);
    lv_obj_set_style_pad_all(ctx->wifi_list, 0, 0);
    lv_obj_set_layout(ctx->wifi_list, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ctx->wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx->wifi_list, 6, 0);
    lv_obj_add_flag(ctx->wifi_list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ctx->wifi_list, LV_DIR_VER);

    ui_create_label(ctx->wifi_list,
        ui_i18n_get(UI_KEY_SET_WIFI_NO_AP),
        ui_i18n_font(UI_FONT_SMALL), UI_COLOR_TEXT_SEC);

    ctx->pwd_card = ui_create_card(page);
    lv_obj_set_width(ctx->pwd_card, lv_pct(100));
    lv_obj_set_flex_flow(ctx->pwd_card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(ctx->pwd_card, 10, 0);
    lv_obj_add_flag(ctx->pwd_card, LV_OBJ_FLAG_HIDDEN);

    ctx->lbl_selected_ssid = ui_create_label(ctx->pwd_card, "",
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_ACCENT);

    lv_obj_t *pwd_row = lv_obj_create(ctx->pwd_card);
    lv_obj_remove_style_all(pwd_row);
    lv_obj_set_style_pad_all(pwd_row, 0, 0);
    lv_obj_set_width(pwd_row, lv_pct(100));
    lv_obj_set_layout(pwd_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pwd_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(pwd_row, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(pwd_row, 10, 0);
    lv_obj_clear_flag(pwd_row, LV_OBJ_FLAG_SCROLLABLE);

    ui_create_label(pwd_row, ui_i18n_get(UI_KEY_SET_WIFI_PASSWORD),
        ui_i18n_font(UI_FONT_SMALL), UI_COLOR_TEXT_SEC);

    ctx->pwd_ta = lv_textarea_create(ctx->pwd_card);
    lv_obj_set_width(ctx->pwd_ta, lv_pct(100));
    lv_textarea_set_placeholder_text(ctx->pwd_ta,
        ui_i18n_get(UI_KEY_SET_WIFI_PASSWORD));
    lv_textarea_set_password_mode(ctx->pwd_ta, true);
    lv_textarea_set_one_line(ctx->pwd_ta, true);
    lv_obj_set_style_border_color(ctx->pwd_ta, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(ctx->pwd_ta, 1, 0);
    lv_obj_set_style_radius(ctx->pwd_ta, 10, 0);
    lv_obj_set_style_pad_ver(ctx->pwd_ta, 10, 0);
    lv_obj_set_style_pad_hor(ctx->pwd_ta, 12, 0);
    lv_obj_set_style_text_font(ctx->pwd_ta, ui_i18n_font(UI_FONT_LABEL), 0);

    ctx->btn_connect_obj = lv_btn_create(ctx->pwd_card);
    lv_obj_set_style_bg_color(ctx->btn_connect_obj, UI_COLOR_GREEN, 0);
    lv_obj_set_style_bg_opa(ctx->btn_connect_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(ctx->btn_connect_obj, UI_BTN_RADIUS, 0);
    lv_obj_set_style_border_width(ctx->btn_connect_obj, 0, 0);
    lv_obj_set_style_pad_ver(ctx->btn_connect_obj, 10, 0);
    lv_obj_set_style_pad_hor(ctx->btn_connect_obj, 32, 0);
    lv_obj_add_event_cb(ctx->btn_connect_obj, on_wifi_connect,
        LV_EVENT_CLICKED, ctx);
    ctx->lbl_connect = ui_create_label(ctx->btn_connect_obj,
        ui_i18n_get(UI_KEY_SET_WIFI_CONNECT),
        ui_i18n_font(UI_FONT_SMALL), lv_color_white());

    lv_obj_t *about_card = create_row_card(page, false);
    create_icon_label_row(about_card, ICON_INFO,
        ui_i18n_get(UI_KEY_SET_ABOUT),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_TEXT);
    ui_create_label(about_card, "v1.0.0", UI_FONT_SMALL, UI_COLOR_TEXT_SEC);

    lv_obj_t *reset_card = create_row_card(page, true);
    lv_obj_add_event_cb(reset_card, on_reset, LV_EVENT_CLICKED, NULL);
    create_icon_label_row(reset_card, ICON_TRASH,
        ui_i18n_get(UI_KEY_SET_RESET),
        ui_i18n_font(UI_FONT_LABEL), UI_COLOR_RED);

    ui_event_subscribe(UI_EVENT_MODEL_UPDATED, on_model_updated, ctx);
    ui_event_subscribe(UI_EVENT_LANG_CHANGED, on_lang_changed_event, ctx);

    printf("[SET] Settings page created\n");
    return page;
}
