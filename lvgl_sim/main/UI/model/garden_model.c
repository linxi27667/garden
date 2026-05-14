#include "garden_model.h"
#include "../core/ui_events.h"
#include "lvgl/lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

static garden_model_t s_model;
static lv_timer_t *s_timer;
static lv_timer_t *s_wifi_scan_timer;
static volatile int s_wifi_scan_pending;

#define LIGHT_LOW_THRESHOLD   300
#define LIGHT_VEG_LOW         200
#define LIGHT_VEG_HIGH        700
#define LIGHT_GARDEN_THRESH   400
#define SMOKE_HIGH_THRESHOLD  60
#define RAIN_WET_THRESHOLD    600

static void sim_tick(lv_timer_t *timer)
{
    (void)timer;
    s_model.light_sensor = 200 + (rand() % 800);
    s_model.smoke_sensor = rand() % 100;
    s_model.rain_sensor = rand() % 1024;
    s_model.soil_moisture = 30 + (rand() % 40);
    s_model.rain_wet = (s_model.rain_sensor > RAIN_WET_THRESHOLD);

    if (s_model.auto_mode) {
        s_model.corridor_light = (s_model.light_sensor < LIGHT_LOW_THRESHOLD);
        s_model.indoor_strip = (s_model.light_sensor < LIGHT_GARDEN_THRESH);
        s_model.water_pump = (s_model.smoke_sensor > SMOKE_HIGH_THRESHOLD);
        if (s_model.veg_light) {
            if (s_model.light_sensor < LIGHT_VEG_LOW) s_model.veg_pwm_duty = 100;
            else if (s_model.light_sensor > LIGHT_VEG_HIGH) s_model.veg_pwm_duty = 10;
            else s_model.veg_pwm_duty = (uint8_t)(100 - s_model.light_sensor / 10);
        }
        if (s_model.rain_wet && s_model.servo_hang) {
            s_model.servo_hang = false;
        }
    }

    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

void garden_model_init(void)
{
    memset(&s_model, 0, sizeof(s_model));
    s_model.auto_mode = true;
    s_model.light_sensor = 512;
    s_model.smoke_sensor = 15;
    s_model.rain_sensor = 200;
    s_model.soil_moisture = 45;
    s_model.rain_wet = false;
    s_model.servo_hang = true;
    s_model.veg_light = false;
    s_model.veg_pwm_duty = 0;
    s_model.wifi_state = WIFI_STATE_IDLE;
}

void garden_model_start(void)
{
    if (!s_timer) {
        s_timer = lv_timer_create(sim_tick, 2000, NULL);
    }
}

void garden_model_stop(void)
{
    if (s_timer) {
        lv_timer_delete(s_timer);
        s_timer = NULL;
    }
}

const garden_model_t *garden_model_get(void)
{
    return &s_model;
}

void garden_model_set_device(const char *device, bool on)
{
    if (!device) return;
    if (strcmp(device, "corridor_light") == 0) s_model.corridor_light = on;
    else if (strcmp(device, "strip_light") == 0) s_model.indoor_strip = on;
    else if (strcmp(device, "water_pump") == 0) s_model.water_pump = on;
    else if (strcmp(device, "mist_cooler") == 0) s_model.mist_cooler = on;
    else if (strcmp(device, "garden_light") == 0) s_model.indoor_strip = on;
    else if (strcmp(device, "exhaust_fan") == 0) s_model.water_pump = on;
    else if (strcmp(device, "veg_light") == 0) {
        s_model.veg_light = on;
        s_model.veg_pwm_duty = on ? 80 : 0;
    }
    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

void garden_model_set_auto(bool on)
{
    s_model.auto_mode = on;
    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

void garden_model_set_servo(bool hang)
{
    s_model.servo_hang = hang;
    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

#ifdef _WIN32
static void parse_netsh_output(const char *output)
{
    s_model.wifi_ap_count = 0;
    const char *p = output;

    while (*p && s_model.wifi_ap_count < WIFI_SCAN_MAX) {
        const char *ssid_hdr = strstr(p, "SSID");
        if (!ssid_hdr) break;

        const char *colon = strchr(ssid_hdr, ':');
        if (!colon) { p = ssid_hdr + 4; continue; }

        if (colon != ssid_hdr + 4) {
            p = ssid_hdr + 4;
            continue;
        }

        colon++;
        while (*colon == ' ') colon++;

        if (*colon == '\r' || *colon == '\n' || *colon == '\0') {
            p = colon;
            continue;
        }

        wifi_ap_t *ap = &s_model.wifi_aps[s_model.wifi_ap_count];
        int i = 0;
        while (*colon && *colon != '\r' && *colon != '\n' && i < WIFI_SSID_MAX - 1) {
            ap->ssid[i++] = *colon++;
        }
        ap->ssid[i] = '\0';

        if (strlen(ap->ssid) == 0) { p = colon; continue; }

        ap->rssi = -70;
        ap->encrypted = true;

        const char *next_ssid = strstr(colon, "SSID");
        const char *search_end = next_ssid ? next_ssid : (colon + strlen(colon));

        const char *sig = strstr(colon, "Signal");
        if (!sig || sig >= search_end) sig = strstr(colon, "\xE4\xBF\xA1\xE5\x8F\xB7");
        if (sig && sig < search_end) {
            const char *sc = strchr(sig, ':');
            if (sc && sc < search_end) {
                sc++;
                while (*sc == ' ') sc++;
                int pct = atoi(sc);
                if (pct > 0) ap->rssi = (int8_t)(pct / 2 - 100);
            }
        }

        const char *auth = strstr(colon, "Authentication");
        if (!auth || auth >= search_end) auth = strstr(colon, "\xE8\xBA\xAB\xE4\xBB\xBD");
        if (!auth || auth >= search_end) auth = strstr(colon, "\xE5\xAE\x89\xE5\x85\xA8");
        if (auth && auth < search_end) {
            const char *ac = strchr(auth, ':');
            if (ac && ac < search_end) {
                ac++;
                while (*ac == ' ') ac++;
                if (strncmp(ac, "Open", 4) == 0 ||
                    strncmp(ac, "\xE5\xBC\x80\xE6\x94\xBE", 6) == 0 ||
                    strncmp(ac, "\xE5\xBC\x80", 3) == 0) {
                    ap->encrypted = false;
                }
            }
        }

        s_model.wifi_ap_count++;
        p = colon;
    }
}

static size_t escape_shell_arg(const char *src, char *dst, size_t dst_size)
{
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 2; i++) {
        char c = src[i];
        if (c == '"' || c == '\\' || c == '$' || c == '`' || c == '!' ||
            c == '&' || c == '|' || c == ';' || c == '(' || c == ')' ||
            c == '<' || c == '>' || c == '\'' || c == '\n' || c == '\r') {
            if (j + 2 >= dst_size) break;
            dst[j++] = '^';
            dst[j++] = c;
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
    return j;
}
#endif

static void wifi_scan_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    if (s_wifi_scan_timer) {
        lv_timer_delete(s_wifi_scan_timer);
        s_wifi_scan_timer = NULL;
    }

    if (!s_wifi_scan_pending) return;
    s_wifi_scan_pending = 0;

#ifdef _WIN32
    FILE *fp = _popen("netsh wlan show networks mode=bssid", "r");
    if (fp) {
        char buf[16384];
        size_t total = 0;
        while (total < sizeof(buf) - 1) {
            size_t n = fread(buf + total, 1, sizeof(buf) - 1 - total, fp);
            if (n == 0) break;
            total += n;
        }
        buf[total] = '\0';
        _pclose(fp);
        parse_netsh_output(buf);
    } else {
        s_model.wifi_ap_count = 0;
    }
#else
    s_model.wifi_ap_count = 0;
#endif

    s_model.wifi_state = WIFI_STATE_IDLE;
    printf("[WIFI] Scan complete, found %d APs\n", s_model.wifi_ap_count);
    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

void garden_model_wifi_scan(void)
{
    s_model.wifi_state = WIFI_STATE_SCANNING;
    s_wifi_scan_pending = 1;
    ui_event_publish(UI_EVENT_MODEL_UPDATED);

    if (s_wifi_scan_timer) {
        lv_timer_del(s_wifi_scan_timer);
    }
    s_wifi_scan_timer = lv_timer_create(wifi_scan_timer_cb, 50, NULL);
    lv_timer_set_repeat_count(s_wifi_scan_timer, 1);
}

void garden_model_wifi_connect(const char *ssid, const char *password)
{
    s_model.wifi_state = WIFI_STATE_CONNECTING;
    strncpy(s_model.wifi_ssid, ssid ? ssid : "", WIFI_SSID_MAX - 1);
    s_model.wifi_ssid[WIFI_SSID_MAX - 1] = '\0';
    ui_event_publish(UI_EVENT_MODEL_UPDATED);

#ifdef _WIN32
    char safe_ssid[128] = {0};
    char safe_pass[128] = {0};
    escape_shell_arg(ssid, safe_ssid, sizeof(safe_ssid));

    char cmd[512];
    if (password && strlen(password) > 0) {
        escape_shell_arg(password, safe_pass, sizeof(safe_pass));
        snprintf(cmd, sizeof(cmd),
            "netsh wlan connect name=\"%s\" ssid=\"%s\" key=\"%s\"",
            safe_ssid, safe_ssid, safe_pass);
    } else {
        snprintf(cmd, sizeof(cmd),
            "netsh wlan connect name=\"%s\"", safe_ssid);
    }
    int ret = system(cmd);
    if (ret == 0) {
        s_model.wifi_state = WIFI_STATE_CONNECTED;
        printf("[WIFI] Connected to %s\n", ssid);
    } else {
        s_model.wifi_state = WIFI_STATE_FAILED;
        printf("[WIFI] Failed to connect to %s (ret=%d)\n", ssid, ret);
    }
#else
    s_model.wifi_state = WIFI_STATE_CONNECTED;
#endif

    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}

void garden_model_wifi_disconnect(void)
{
#ifdef _WIN32
    system("netsh wlan disconnect");
#endif
    s_model.wifi_state = WIFI_STATE_IDLE;
    s_model.wifi_ssid[0] = '\0';
    printf("[WIFI] Disconnected\n");
    ui_event_publish(UI_EVENT_MODEL_UPDATED);
}
