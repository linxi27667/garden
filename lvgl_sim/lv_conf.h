#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

#define LV_COLOR_DEPTH 32
#define LV_COLOR_SCREEN_TRANSP 1

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF 1

#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1

#define LV_MEM_SIZE (8 * 1024 * 1024)
#define LV_MEM_ADR 0

#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

#define LV_USE_FLEX 1
#define LV_USE_GRID 1

#define LV_USE_ANIM 1

#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BTN 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMG 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_TEXTAREA 1
#define LV_USE_TABLE 1

#define LV_USE_MSGBOX 1
#define LV_USE_SPAN 1
#define LV_USE_TABVIEW 1
#define LV_USE_TILEVIEW 1

#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 0

#define LV_USE_SNAPSHOT 1

#define LV_USE_SYSMON 0
#define LV_USE_PERF_MONITOR 0

#define LV_TICK_CUSTOM 1

#define LV_USE_BINFONT 1

#define LV_USE_TINY_TTF 1

#define LV_USE_SDL              1
#if LV_USE_SDL
    #define LV_SDL_INCLUDE_PATH     <SDL2/SDL.h>
    #define LV_SDL_RENDER_MODE      LV_DISPLAY_RENDER_MODE_PARTIAL
    #define LV_SDL_BUF_COUNT        1
    #define LV_SDL_ACCELERATED      1
    #define LV_SDL_FULLSCREEN       0
    #define LV_SDL_DIRECT_EXIT      1
#endif

#endif
