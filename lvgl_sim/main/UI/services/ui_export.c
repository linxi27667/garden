#include "ui_export.h"
#include "lvgl/lvgl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;
    uint32_t bfSize;
    uint16_t bfReserved1;
    uint16_t bfReserved2;
    uint32_t bfOffBits;
} bmp_file_header_t;

typedef struct {
    uint32_t biSize;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes;
    uint16_t biBitCount;
    uint32_t biCompression;
    uint32_t biSizeImage;
    int32_t  biXPelsPerMeter;
    int32_t  biYPelsPerMeter;
    uint32_t biClrUsed;
    uint32_t biClrImportant;
} bmp_info_header_t;
#pragma pack(pop)

static int save_bmp(const char *filename, const uint8_t *bgra_data,
    uint32_t w, uint32_t h, uint32_t stride)
{
    uint32_t row_size = w * 3;
    uint32_t padding = (4 - (row_size % 4)) % 4;
    uint32_t padded_row = row_size + padding;
    uint32_t img_size = padded_row * h;

    bmp_file_header_t fh = {0};
    fh.bfType = 0x4D42;
    fh.bfSize = sizeof(fh) + sizeof(bmp_info_header_t) + img_size;
    fh.bfOffBits = sizeof(fh) + sizeof(bmp_info_header_t);

    bmp_info_header_t ih = {0};
    ih.biSize = sizeof(ih);
    ih.biWidth = (int32_t)w;
    ih.biHeight = (int32_t)h;
    ih.biPlanes = 1;
    ih.biBitCount = 24;
    ih.biSizeImage = img_size;

    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("[ui_export] Cannot open file: %s\n", filename);
        return -1;
    }

    fwrite(&fh, sizeof(fh), 1, f);
    fwrite(&ih, sizeof(ih), 1, f);

    uint8_t pad[3] = {0};
    for (int32_t y = (int32_t)h - 1; y >= 0; y--) {
        const uint8_t *row = bgra_data + y * stride;
        for (uint32_t x = 0; x < w; x++) {
            uint8_t bgr[3] = { row[x*4+0], row[x*4+1], row[x*4+2] };
            fwrite(bgr, 3, 1, f);
        }
        if (padding) fwrite(pad, padding, 1, f);
    }

    fclose(f);
    return 0;
}

void ui_export_screenshot(const char *filename)
{
    lv_obj_t *screen = lv_screen_active();
    lv_draw_buf_t *draw_buf = lv_snapshot_take(screen, LV_COLOR_FORMAT_ARGB8888);

    if (draw_buf == NULL) {
        printf("[ui_export] Screenshot failed: lv_snapshot_take returned NULL\n");
        return;
    }

    uint32_t w = draw_buf->header.w;
    uint32_t h = draw_buf->header.h;
    uint32_t stride = draw_buf->header.stride;

    int ret = save_bmp(filename, draw_buf->data, w, h, stride);
    if (ret == 0) {
        printf("[ui_export] Screenshot saved: %s (%dx%d)\n", filename, w, h);
    } else {
        printf("[ui_export] Failed to save screenshot\n");
    }

    lv_draw_buf_destroy(draw_buf);
}
