#ifndef UI_EVENTS_H
#define UI_EVENTS_H

#include <stdint.h>

typedef enum {
    UI_EVENT_LANG_CHANGED,
    UI_EVENT_MODEL_UPDATED,
    UI_EVENT_PAGE_SWITCHED,
    UI_EVENT_COUNT
} ui_event_type_t;

typedef void (*ui_event_cb_t)(void *user_data);

#define UI_UNSUB_ANY ((void *)(intptr_t)-1)

void ui_events_init(void);
void ui_event_subscribe(ui_event_type_t evt, ui_event_cb_t cb, void *user_data);
void ui_event_unsubscribe(ui_event_type_t evt, ui_event_cb_t cb, void *user_data);
void ui_event_publish(ui_event_type_t evt);

#endif
