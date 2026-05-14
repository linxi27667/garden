#include "lvgl/lvgl.h"
#include "UI/core/ui_app.h"
#include "UI/services/ui_export.h"
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#include <unistd.h>
#endif

static void hal_init(int32_t w, int32_t h)
{
    lv_display_t *disp = lv_sdl_window_create(w, h);
    lv_display_set_default(disp);

    lv_indev_t *mouse = lv_sdl_mouse_create();
    lv_indev_set_display(mouse, disp);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);

    lv_indev_t *mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, g);

    lv_indev_t *kb = lv_sdl_keyboard_create();
    lv_indev_set_display(kb, disp);
    lv_indev_set_group(kb, g);

    printf("[HAL] Display %dx%d, mouse + wheel + keyboard\n", w, h);
}

static void render_ms(int ms)
{
    for (int i = 0; i < ms / 5; i++) {
        lv_timer_handler();
        ui_app_poll();
#ifdef _MSC_VER
        Sleep(5);
#else
        usleep(5000);
#endif
    }
}

static int check_screenshot_arg(int argc, char **argv, char *out_path, int *out_timeout)
{
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--export-screenshot=", 20) == 0) {
            strncpy(out_path, argv[i] + 20, 511);
            out_path[511] = '\0';
            *out_timeout = 100;
            for (int j = 1; j < argc; j++) {
                if (strncmp(argv[j], "--timeout=", 10) == 0) {
                    *out_timeout = atoi(argv[j] + 10);
                    if (*out_timeout < 10) *out_timeout = 10;
                }
            }
            return 1;
        }
    }
    return 0;
}

static void parse_resolution(int argc, char **argv, int *out_w, int *out_h)
{
    *out_w = 1024;
    *out_h = 600;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "--resolution=", 13) == 0) {
            sscanf(argv[i] + 13, "%dx%d", out_w, out_h);
        }
    }
}

int SDL_main(int argc, char **argv)
{
    int disp_w = 1024, disp_h = 600;
    parse_resolution(argc, argv, &disp_w, &disp_h);

    lv_init();
    hal_init(disp_w, disp_h);
    ui_app_init();

    char screenshot_path[512] = {0};
    int screenshot_timeout = 100;

    if (check_screenshot_arg(argc, argv, screenshot_path, &screenshot_timeout)) {
        printf("[screenshot] Rendering UI for %d ms...\n", screenshot_timeout);
        render_ms(screenshot_timeout);
        ui_export_screenshot(screenshot_path);
        printf("[screenshot] Done. Exiting.\n");
        return 0;
    }

    printf("Smart Garden LVGL Simulator started (%dx%d SDL).\n", disp_w, disp_h);
    printf("Click bottom tab bar to switch pages. Close window to exit.\n");

    while (1) {
        lv_timer_handler();
        ui_app_poll();
#ifdef _MSC_VER
        Sleep(5);
#else
        usleep(5000);
#endif
    }

    return 0;
}
