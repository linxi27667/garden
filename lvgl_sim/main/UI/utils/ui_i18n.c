#include "ui_i18n.h"
#include "fonts/ui_font.h"
#include "../core/ui_events.h"
#include <string.h>

static ui_lang_t s_lang = UI_LANG_ZH;

static const char *s_zh[] = {
    [0]  = "\xE6\x95\xB0\xE6\x8D\xAE",
    [1]  = "\xE6\x8E\xA7\xE5\x88\xB6",
    [2]  = "\xE8\xAE\xBE\xE7\xBD\xAE",
    [3]  = "\xE5\x9C\xA8\xE7\xBA\xBF",
    [4]  = "\xE7\xA6\xBB\xE7\xBA\xBF",
    [5]  = "\xE6\x99\xBA\xE8\x83\xBD\xE8\x8F\x9C\xE5\x9B\xAD",
    [6]  = "ESP32-P4",
    [7]  = "\xE5\xAE\x9E\xE6\x97\xB6\xE6\x95\xB0\xE6\x8D\xAE",
    [8]  = "\xE5\x85\x89\xE6\x95\x8F\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8",
    [9]  = "\xE7\x83\x9F\xE9\x9B\xBE\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8",
    [10] = "\xE9\x9B\xA8\xE6\xBB\xB4\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8",
    [11] = "\xE8\xB5\xB0\xE5\xBB\x8A\xE7\x81\xAF",
    [12] = "\xE5\xAE\xA4\xE5\x86\x85\xE7\x81\xAF\xE5\xB8\xA6",
    [13] = "\xE5\xBE\xAA\xE7\x8E\xAF\xE6\xB0\xB4\xE6\xB3\xB5",
    [14] = "\xE9\x9B\xBE\xE5\x8C\x96\xE9\x99\x8D\xE6\xB8\xA9\xE5\x99\xA8",
    [15] = "\xE8\xAE\xBE\xE5\xA4\x87",
    [16] = "\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8",
    [17] = "\xE8\x87\xAA\xE5\x8A\xA8\xE6\xA8\xA1\xE5\xBC\x8F",
    [18] = "\xE6\x89\x8B\xE5\x8A\xA8",
    [19] = "\xE5\xBC\x80",
    [20] = "\xE5\x85\xB3",
    [21] = "\xE8\xAE\xBE\xE7\xBD\xAE",
    [22] = "\xE8\xAF\xAD\xE8\xA8\x80",
    [23] = "\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87",
    [24] = "WiFi",
    [25] = "\xE6\x89\xAB\xE6\x8F\x8F",
    [26] = "\xE6\x89\xAB\xE6\x8F\x8F\xE4\xB8\xAD...",
    [27] = "\xE8\xBF\x9E\xE6\x8E\xA5",
    [28] = "\xE6\x96\xAD\xE5\xBC\x80",
    [29] = "\xE5\xB7\xB2\xE8\xBF\x9E\xE6\x8E\xA5",
    [30] = "\xE5\xAF\x86\xE7\xA0\x81",
    [31] = "\xE6\x9C\xAA\xE6\x89\xBE\xE5\x88\xB0\xE7\xBD\x91\xE7\xBB\x9C",
    [32] = "\xE8\xBF\x9E\xE6\x8E\xA5\xE5\xA4\xB1\xE8\xB4\xA5",
    [33] = "\xE5\x85\xB3\xE4\xBA\x8E",
    [34] = "\xE7\x89\x88\xE6\x9C\xAC",
    [35] = "\xE6\x81\xA2\xE5\xA4\x8D\xE5\x87\xBA\xE5\x8E\x82",
    [36] = "\xE7\xA1\xAE\xE8\xAE\xA4\xE9\x87\x8D\xE7\xBD\xAE\xE6\x89\x80\xE6\x9C\x89\xE6\x95\xB0\xE6\x8D\xAE?",
    [37] = "\xE6\x8E\xA7\xE5\x88\xB6\xE9\x9D\xA2\xE6\x9D\xBF",
    [38] = "\xE5\x9C\xBA\xE6\x99\xAF\xE6\xA8\xA1\xE5\xBC\x8F",
    [39] = "\xE6\x97\xA9\xE5\xAE\x89",
    [40] = "\xE6\x99\x9A\xE5\xAE\x89",
    [41] = "\xE7\xA6\xBB\xE5\xAE\xB6",
    [42] = "lux",
    [43] = "ppm",
    [44] = "%",
    [45] = "\xE7\x85\xA7\xE6\x98\x8E",
    [46] = "\xE8\xAE\xBE\xE5\xA4\x87",
    [47] = "\xE4\xBC\xA0\xE6\x84\x9F\xE5\x99\xA8",
    [48] = "\xE8\x87\xAA\xE5\x8A\xA8\xE6\xA8\xA1\xE5\xBC\x8F",
    [49] = "\xE8\xB5\xB0\xE5\xBB\x8A\xE7\x81\xAF",
    [50] = "\xE7\x81\xAF\xE5\xB8\xA6",
    [51] = "\xE8\x8F\x9C\xE7\x81\xAF",
    [52] = "\xE6\x8E\x92\xE6\xB0\x94\xE6\x89\x87",
    [53] = "\xE6\x99\xBE\xE8\xA1\xA3\xE6\x9E\xB6",
    [54] = "\xE5\xB7\xB2\xE6\x99\xBE\xE5\x87\xBA",
    [55] = "\xE5\xB7\xB2\xE6\x94\xB6\xE5\x9B\x9E",
    [56] = "\xE6\x99\xBE\xE5\x87\xBA",
    [57] = "\xE6\x94\xB6\xE5\x9B\x9E",
    [58] = "\xE5\x9C\x9F\xE5\xA3\xA4\xE6\xB9\xBF\xE5\xBA\xA6",
    [59] = "\xE5\x85\x89\xE7\x85\xA7",
    [60] = "\xE9\x9B\xA8\xE6\xBB\xB4",
    [61] = "\xE4\xB8\x8B\xE9\x9B\xA8\xE4\xB8\xAD",
    [62] = "\xE6\x9C\xAA\xE4\xB8\x8B\xE9\x9B\xA8",
    [63] = "\xE8\x87\xAA\xE5\x8A\xA8",
    [64] = "\xE6\x89\x8B\xE5\x8A\xA8",
};

static const char *s_en[] = {
    [0]  = "Data",
    [1]  = "Control",
    [2]  = "Settings",
    [3]  = "Online",
    [4]  = "Offline",
    [5]  = "Smart Garden",
    [6]  = "ESP32-P4",
    [7]  = "Real-time Data",
    [8]  = "Light Sensor",
    [9]  = "Smoke Sensor",
    [10] = "Rain Sensor",
    [11] = "Corridor Light",
    [12] = "Indoor Strip Light",
    [13] = "Water Pump",
    [14] = "Mist Cooler",
    [15] = "Devices",
    [16] = "Sensors",
    [17] = "Auto Mode",
    [18] = "Manual",
    [19] = "On",
    [20] = "Off",
    [21] = "Settings",
    [22] = "Language",
    [23] = "English",
    [24] = "WiFi",
    [25] = "Scan",
    [26] = "Scanning...",
    [27] = "Connect",
    [28] = "Disconnect",
    [29] = "Connected",
    [30] = "Password",
    [31] = "No networks found",
    [32] = "Connection failed",
    [33] = "About",
    [34] = "Version",
    [35] = "Reset",
    [36] = "Reset all data?",
    [37] = "Control Panel",
    [38] = "Scene Mode",
    [39] = "Morning",
    [40] = "Night",
    [41] = "Away",
    [42] = "lux",
    [43] = "ppm",
    [44] = "%",
    [45] = "Lighting",
    [46] = "Devices",
    [47] = "Sensors",
    [48] = "Auto Mode",
    [49] = "Corridor Light",
    [50] = "Strip Light",
    [51] = "Veg Light",
    [52] = "Exhaust Fan",
    [53] = "Clothes Rack",
    [54] = "Hung Out",
    [55] = "Collected",
    [56] = "Hang Out",
    [57] = "Collect",
    [58] = "Soil Moisture",
    [59] = "Light",
    [60] = "Rain",
    [61] = "Raining",
    [62] = "Dry",
    [63] = "Auto",
    [64] = "Manual",
};

void ui_i18n_init(void)
{
    s_lang = UI_LANG_ZH;
}

const char *ui_i18n_get(ui_i18n_key_t key)
{
    if (key < 0 || key >= UI_KEY_COUNT) return "";
    return s_lang == UI_LANG_ZH ? s_zh[key] : s_en[key];
}

ui_lang_t ui_i18n_get_lang(void)
{
    return s_lang;
}

void ui_i18n_set_lang(ui_lang_t lang)
{
    s_lang = lang;
    ui_event_publish(UI_EVENT_LANG_CHANGED);
}

const lv_font_t *ui_i18n_font(const lv_font_t *fallback)
{
    uint8_t req_size = 14;
    if (fallback == &lv_font_montserrat_12) {
        req_size = 12;
    } else if (fallback == &lv_font_montserrat_14) {
        req_size = 14;
    } else if (fallback == &lv_font_montserrat_16) {
        req_size = 16;
    } else if (fallback == &lv_font_montserrat_20) {
        req_size = 20;
    } else if (fallback == &lv_font_montserrat_24) {
        req_size = 24;
    } else if (fallback == &lv_font_montserrat_36) {
        req_size = 36;
    } else if (fallback == &lv_font_montserrat_48) {
        req_size = 48;
    }
    if (s_lang == UI_LANG_ZH) {
        return ui_font_cn(req_size);
    }
    return fallback;
}
