#include "ui_theme.h"

static lv_style_transition_dsc_t trans_btn;

static const lv_style_prop_t btn_t_props[] = {
    LV_STYLE_BG_COLOR,
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    0
};

void ui_theme_init(void)
{
    lv_style_transition_dsc_init(&trans_btn, btn_t_props,
        lv_anim_path_ease_out, 200, 0, NULL);
}
