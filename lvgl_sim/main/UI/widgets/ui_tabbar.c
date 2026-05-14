#include "ui_tabbar.h"
#include "../core/ui_styles.h"
#include "../services/ui_i18n.h"
#include "../services/ui_icons.h"
#include "../fonts/ui_font.h"
#include <string.h>
#include <stdio.h>

#define MAX_TABS 3

typedef struct {
    ui_tab_cb_t cb;
    lv_obj_t *tab_btns[MAX_TABS];
    lv_obj_t *tab_icons[MAX_TABS];
    lv_obj_t *tab_lbls[MAX_TABS];
    lv_obj_t *tab_indicators[MAX_TABS];
    int active_idx;
} tabbar_ctx_t;

static void tab_clicked(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    tabbar_ctx_t *ctx = (tabbar_ctx_t *)lv_obj_get_user_data(btn);
    if (!ctx) return;

    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx >= 0 && idx < MAX_TABS && ctx->cb) {
        ctx->cb((void *)(intptr_t)idx);
    }
}

void ui_tabbar_set_active(lv_obj_t *tabbar, int active)
{
    tabbar_ctx_t *ctx = (tabbar_ctx_t *)lv_obj_get_user_data(tabbar);
    if (!ctx) return;

    ctx->active_idx = active;
    for (int i = 0; i < MAX_TABS; i++) {
        lv_obj_t *icon = ctx->tab_icons[i];
        lv_obj_t *lbl = ctx->tab_lbls[i];
        lv_obj_t *indicator = ctx->tab_indicators[i];

        if (i == ctx->active_idx) {
            if (icon) lv_obj_set_style_text_color(icon, UI_COLOR_ACCENT, 0);
            if (lbl) lv_obj_set_style_text_color(lbl, UI_COLOR_ACCENT, 0);
            if (indicator) lv_obj_set_style_opa(indicator, LV_OPA_COVER, 0);
        } else {
            if (icon) lv_obj_set_style_text_color(icon, UI_COLOR_TEXT_SEC, 0);
            if (lbl) lv_obj_set_style_text_color(lbl, UI_COLOR_TEXT_SEC, 0);
            if (indicator) lv_obj_set_style_opa(indicator, LV_OPA_TRANSP, 0);
        }
    }
}

void ui_tabbar_refresh_text(lv_obj_t *tabbar)
{
    tabbar_ctx_t *ctx = (tabbar_ctx_t *)lv_obj_get_user_data(tabbar);
    if (!ctx) return;

    for (int i = 0; i < MAX_TABS; i++) {
        if (ctx->tab_lbls[i]) {
            lv_label_set_text(ctx->tab_lbls[i],
                ui_i18n_get((ui_i18n_key_t)(UI_KEY_TAB_DATA + i)));
        }
    }
}

static void tabbar_delete_cb(lv_event_t *e)
{
    lv_obj_t *tabbar = lv_event_get_target(e);
    tabbar_ctx_t *ctx = (tabbar_ctx_t *)lv_obj_get_user_data(tabbar);
    if (ctx) {
        lv_free(ctx);
        lv_obj_set_user_data(tabbar, NULL);
    }
}

lv_obj_t *ui_tabbar_create(lv_obj_t *parent, ui_tab_cb_t cb)
{
    tabbar_ctx_t *ctx = lv_malloc(sizeof(tabbar_ctx_t));
    memset(ctx, 0, sizeof(*ctx));
    ctx->cb = cb;
    ctx->active_idx = 0;

    lv_obj_t *tabbar = lv_obj_create(parent);
    lv_obj_set_user_data(tabbar, ctx);
    lv_obj_set_size(tabbar, lv_pct(100), UI_TAB_HEIGHT);
    lv_obj_set_style_bg_color(tabbar, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(tabbar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(tabbar, 0, 0);
    lv_obj_set_style_border_side(tabbar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_border_color(tabbar, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(tabbar, 1, 0);
    lv_obj_set_style_pad_all(tabbar, 0, 0);
    lv_obj_set_layout(tabbar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(tabbar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(tabbar, LV_FLEX_ALIGN_SPACE_AROUND,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(tabbar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(tabbar, tabbar_delete_cb, LV_EVENT_DELETE, NULL);

    for (int i = 0; i < MAX_TABS; i++) {
        const char *text = ui_i18n_get((ui_i18n_key_t)(UI_KEY_TAB_DATA + i));

        /* Each tab is a button itself - clickable, with column layout for icon+label */
        lv_obj_t *btn = lv_btn_create(tabbar);
        lv_obj_set_user_data(btn, ctx);
        lv_obj_set_flex_grow(btn, 1);
        lv_obj_set_height(btn, lv_pct(100));
        lv_obj_remove_style_all(btn);
        lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
        lv_obj_set_style_bg_color(btn, UI_COLOR_TAB_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_bg_opa(btn, LV_OPA_40, LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 12, LV_STATE_PRESSED);
        lv_obj_set_style_border_width(btn, 0, 0);
        lv_obj_set_style_pad_all(btn, 6, 0);
        lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER,
            LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_row(btn, 4, 0);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_shadow_width(btn, 0, 0);
        ctx->tab_btns[i] = btn;

        lv_obj_add_event_cb(btn, tab_clicked, LV_EVENT_CLICKED, (void *)(intptr_t)i);

        /* Icon */
        lv_obj_t *icon = lv_label_create(btn);
        lv_label_set_text(icon, i == 0 ? ICON_CHART : (i == 1 ? ICON_CONTROL : ICON_SETTINGS));
        lv_obj_set_style_text_font(icon, ui_font_icon(24), 0);
        lv_obj_set_style_text_color(icon, i == 0 ? UI_COLOR_ACCENT : UI_COLOR_TEXT_SEC, 0);
        ctx->tab_icons[i] = icon;

        /* Label */
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, text);
        lv_obj_set_style_text_font(lbl, ui_i18n_font(UI_FONT_14), 0);
        lv_obj_set_style_text_color(lbl, i == 0 ? UI_COLOR_ACCENT : UI_COLOR_TEXT_SEC, 0);
        ctx->tab_lbls[i] = lbl;

        /* Indicator bar at bottom */
        lv_obj_t *indicator = lv_obj_create(btn);
        lv_obj_remove_style_all(indicator);
        lv_obj_set_size(indicator, 24, 3);
        lv_obj_set_style_bg_color(indicator, UI_COLOR_ACCENT, 0);
        lv_obj_set_style_bg_opa(indicator, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(indicator, 2, 0);
        lv_obj_set_style_opa(indicator, i == 0 ? LV_OPA_COVER : LV_OPA_TRANSP, 0);
        ctx->tab_indicators[i] = indicator;
    }

    return tabbar;
}
