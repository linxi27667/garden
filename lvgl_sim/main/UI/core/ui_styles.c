#include "ui_styles.h"
#include "../utils/ui_theme.h"
#include "../services/ui_i18n.h"
#include "../fonts/ui_font.h"
#include <string.h>

lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text,
    const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text ? text : "");
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

lv_obj_t *ui_create_icon(lv_obj_t *parent, const char *icon,
    const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, icon ? icon : "");
    lv_obj_set_style_text_font(lbl, font, 0);
    lv_obj_set_style_text_color(lbl, color, 0);
    return lbl;
}

lv_obj_t *ui_create_dot(lv_obj_t *parent, lv_color_t color, int size)
{
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, size, size);
    lv_obj_set_style_bg_color(dot, color, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return dot;
}

lv_obj_t *ui_create_switch(lv_obj_t *parent, lv_color_t active_color)
{
    lv_obj_t *sw = lv_switch_create(parent);
    lv_obj_set_style_bg_color(sw, UI_COLOR_BORDER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sw, active_color, LV_PART_INDICATOR | LV_STATE_CHECKED);
    return sw;
}

lv_obj_t *ui_create_card(lv_obj_t *parent)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_remove_style_all(card);
    lv_obj_set_width(card, lv_pct(100));
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(card, UI_COLOR_CARD, 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(card, UI_CARD_RADIUS, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 16, 0);
    lv_obj_set_style_pad_gap(card, 8, 0);
    lv_obj_set_layout(card, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    return card;
}

void ui_apply_card_shadow(lv_obj_t *card)
{
    lv_obj_set_style_shadow_width(card, 8, 0);
    lv_obj_set_style_shadow_opa(card, LV_OPA_10, 0);
    lv_obj_set_style_shadow_color(card, lv_color_black(), 0);
    lv_obj_set_style_shadow_offset_y(card, 2, 0);
}

lv_obj_t *ui_create_page(lv_obj_t *parent, lv_event_cb_t delete_cb, void *user_data)
{
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_remove_style_all(page);
    lv_obj_set_width(page, lv_pct(100));
    lv_obj_set_height(page, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(page, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_hor(page, 20, 0);
    lv_obj_set_style_pad_ver(page, 16, 0);
    lv_obj_set_layout(page, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(page, LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(page, 12, 0);
    if (delete_cb && user_data) {
        lv_obj_add_event_cb(page, delete_cb, LV_EVENT_DELETE, user_data);
    }
    return page;
}

lv_obj_t *ui_create_title_row(lv_obj_t *parent, const char *icon,
    const char *title)
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

    ui_create_icon(row, icon, ui_font_icon(20), UI_COLOR_ACCENT);
    ui_create_label(row, title, ui_i18n_font(UI_FONT_TITLE), UI_COLOR_TEXT);
    return row;
}

void ui_styles_init(void)
{
}
