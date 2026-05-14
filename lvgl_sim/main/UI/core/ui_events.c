#include "ui_events.h"
#include <string.h>
#include <stdio.h>

#define MAX_SUBSCRIBERS 16

typedef struct {
    ui_event_cb_t cb;
    void *user_data;
} subscriber_t;

static const char *s_event_names[] = {
    "LANG_CHANGED",
    "MODEL_UPDATED",
    "PAGE_SWITCHED",
};

static struct {
    subscriber_t subs[MAX_SUBSCRIBERS];
    int count;
} s_events[UI_EVENT_COUNT];

void ui_events_init(void)
{
    memset(s_events, 0, sizeof(s_events));
}

void ui_event_subscribe(ui_event_type_t evt, ui_event_cb_t cb, void *user_data)
{
    if (evt >= UI_EVENT_COUNT || !cb) return;
    for (int i = 0; i < s_events[evt].count; i++) {
        if (s_events[evt].subs[i].cb == cb && s_events[evt].subs[i].user_data == user_data) return;
    }
    if (s_events[evt].count >= MAX_SUBSCRIBERS) return;
    s_events[evt].subs[s_events[evt].count].cb = cb;
    s_events[evt].subs[s_events[evt].count].user_data = user_data;
    s_events[evt].count++;
}

void ui_event_unsubscribe(ui_event_type_t evt, ui_event_cb_t cb, void *user_data)
{
    if (evt >= UI_EVENT_COUNT) return;
    for (int i = 0; i < s_events[evt].count; i++) {
        if (s_events[evt].subs[i].cb == cb &&
            (user_data == UI_UNSUB_ANY || s_events[evt].subs[i].user_data == user_data)) {
            s_events[evt].subs[i] = s_events[evt].subs[s_events[evt].count - 1];
            s_events[evt].count--;
            return;
        }
    }
}

void ui_event_publish(ui_event_type_t evt)
{
    if (evt >= UI_EVENT_COUNT) return;
    printf("[EVENT] %s (%d subscribers)\n", s_event_names[evt], s_events[evt].count);
    int count = s_events[evt].count;
    for (int i = 0; i < count; i++) {
        s_events[evt].subs[i].cb(s_events[evt].subs[i].user_data);
    }
}
