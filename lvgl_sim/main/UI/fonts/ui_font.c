#include "ui_font.h"
#include <stdio.h>
#include <stdlib.h>

#if LV_USE_BINFONT

static const lv_font_t *s_fa_solid = NULL;
static const lv_font_t *s_fa_regular = NULL;

#endif

#if LV_USE_TINY_TTF

static lv_font_t *s_cn_12 = NULL;
static lv_font_t *s_cn_14 = NULL;
static lv_font_t *s_cn_16 = NULL;
static lv_font_t *s_cn_20 = NULL;
static lv_font_t *s_cn_24 = NULL;
static lv_font_t *s_cn_36 = NULL;
static lv_font_t *s_cn_48 = NULL;

static void *load_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        lv_log("ui_font: failed to open %s\n", path);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (len <= 0) { fclose(f); return NULL; }
    void *data = malloc(len);
    if (!data) { fclose(f); return NULL; }
    fread(data, 1, len, f);
    fclose(f);
    *out_size = len;
    return data;
}

static lv_font_t *s_fa_solid_ttf = NULL;
static lv_font_t *s_fa_solid_16 = NULL;
static lv_font_t *s_fa_solid_24 = NULL;

void ui_font_init(void)
{
    const char *cn_paths[] = {
        "NotoSansSC.ttf",
        "main/UI/fonts/NotoSansSC-Regular.ttf",
    };

    size_t ttf_size = 0;
    void *ttf_data = NULL;

    for (int i = 0; i < 2 && ttf_data == NULL; i++) {
        ttf_data = load_file(cn_paths[i], &ttf_size);
    }

    if (!ttf_data) {
        lv_log("ui_font: NotoSansSC TTF not found\n");
    } else {
        s_cn_12 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 12);
        s_cn_14 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 14);
        s_cn_16 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 16);
        s_cn_20 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 20);
        s_cn_24 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 24);
        s_cn_36 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 36);
        s_cn_48 = lv_tiny_ttf_create_data(ttf_data, ttf_size, 48);
        lv_log("ui_font: NotoSansSC loaded OK\n");
    }

    const char *fa_paths[] = {
        "fa-solid-900.ttf",
        "main/UI/fonts/fa-solid-900.ttf",
    };

    size_t fa_size = 0;
    void *fa_data = NULL;

    for (int i = 0; i < 2 && fa_data == NULL; i++) {
        fa_data = load_file(fa_paths[i], &fa_size);
    }

    if (!fa_data) {
        lv_log("ui_font: FontAwesome TTF not found\n");
    } else {
        s_fa_solid_ttf = lv_tiny_ttf_create_data(fa_data, fa_size, 20);
        s_fa_solid_16 = lv_tiny_ttf_create_data(fa_data, fa_size, 16);
        s_fa_solid_24 = lv_tiny_ttf_create_data(fa_data, fa_size, 24);
        lv_log("ui_font: FontAwesome loaded OK\n");
    }
}

static uint8_t nearest_cn_size(uint8_t size)
{
    if (size <= 13) return 12;
    if (size <= 15) return 14;
    if (size <= 18) return 16;
    if (size <= 22) return 20;
    if (size <= 30) return 24;
    if (size <= 42) return 36;
    return 48;
}

const lv_font_t *ui_font_cn(uint8_t size)
{
#if LV_USE_TINY_TTF
    uint8_t ns = nearest_cn_size(size);
    switch (ns) {
        case 12: return s_cn_12 ? s_cn_12 : UI_FONT_12;
        case 14: return s_cn_14 ? s_cn_14 : UI_FONT_14;
        case 16: return s_cn_16 ? s_cn_16 : UI_FONT_16;
        case 20: return s_cn_20 ? s_cn_20 : UI_FONT_20;
        case 24: return s_cn_24 ? s_cn_24 : UI_FONT_24;
        case 36: return s_cn_36 ? s_cn_36 : UI_FONT_36;
        case 48: return s_cn_48 ? s_cn_48 : UI_FONT_48;
        default:  return s_cn_14 ? s_cn_14 : UI_FONT_14;
    }
#else
    switch (size) {
        case 12: return UI_FONT_12;
        case 14: return UI_FONT_14;
        case 16: return UI_FONT_16;
        case 20: return UI_FONT_20;
        case 24: return UI_FONT_24;
        case 36: return UI_FONT_36;
        case 48: return UI_FONT_48;
        default: return UI_FONT_14;
    }
#endif
}

const lv_font_t *ui_font_icon(uint8_t size)
{
#if LV_USE_TINY_TTF
    if (size <= 18 && s_fa_solid_16) return s_fa_solid_16;
    if (size <= 22 && s_fa_solid_ttf) return s_fa_solid_ttf;
    if (s_fa_solid_24) return s_fa_solid_24;
#endif
    if (size <= 18) return UI_FONT_16;
    if (size <= 22) return UI_FONT_20;
    return UI_FONT_24;
}

#else

static uint8_t nearest_cn_size(uint8_t size)
{
    if (size <= 13) return 12;
    if (size <= 15) return 14;
    if (size <= 18) return 16;
    if (size <= 22) return 20;
    if (size <= 30) return 24;
    if (size <= 42) return 36;
    return 48;
}

void ui_font_init(void) {}

const lv_font_t *ui_font_cn(uint8_t size)
{
    switch (nearest_cn_size(size)) {
        case 12: return UI_FONT_12;
        case 14: return UI_FONT_14;
        case 16: return UI_FONT_16;
        case 20: return UI_FONT_20;
        case 24: return UI_FONT_24;
        case 36: return UI_FONT_36;
        case 48: return UI_FONT_48;
        default: return UI_FONT_14;
    }
}

const lv_font_t *ui_font_icon(uint8_t size)
{
    if (size <= 18) return UI_FONT_16;
    if (size <= 22) return UI_FONT_20;
    return UI_FONT_24;
}

#endif
