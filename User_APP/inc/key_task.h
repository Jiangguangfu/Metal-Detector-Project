#ifndef KEY_TASK_H
#define KEY_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    KEY_ID_1 = 1,
    KEY_ID_2 = 2,
    KEY_ID_3 = 3,
} key_id_t;

typedef enum {
    KEY_EVENT_SINGLE_CLICK = 1,
    KEY_EVENT_DOUBLE_CLICK = 2,
    KEY_EVENT_LONG_PRESS = 3,
} key_event_type_t;

typedef struct {
    uint8_t key_id;
    uint8_t event_type;
    uint16_t reserved;
    uint32_t tick_ms;
} key_event_t;

typedef void (*key_event_cb_t)(const key_event_t *evt, void *user);

bool Key_Task_Create(void);
bool Key_RegisterCallback(key_event_cb_t cb, void *user);
void Key_Task_1msTickISR(void);

#ifdef __cplusplus
}
#endif

#endif /* KEY_TASK_H */
