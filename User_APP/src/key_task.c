#include "key_task.h"
#include "user_config.h"
#include "log_task.h"
#include "main.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define KEY_CB_MAX 4

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t stable;
    uint8_t raw_last;
    uint16_t debounce_cnt;
    uint16_t press_ms;
    uint8_t long_sent;
    uint8_t click_cnt;
    uint16_t click_window;
} key_ctx_t;

static osThreadId_t keyTaskHandle;
static StackType_t keyTaskStack[USER_CONFIG_KEY_TASK_STACK_SIZE];
static StaticTask_t keyTaskTCB;
static StaticQueue_t keyEvtQueueCtrl;
static uint8_t keyEvtQueueStorage[USER_CONFIG_KEY_EVT_QUEUE_LEN * sizeof(key_event_t)];
static QueueHandle_t keyEvtQueue;
static volatile uint32_t s_tick_ms;
static key_event_cb_t s_cbs[KEY_CB_MAX];
static void *s_cb_users[KEY_CB_MAX];

static key_ctx_t s_keys[3] = {
    { .port = BUTTON1_GPIO_Port, .pin = BUTTON1_Pin },
    { .port = BUTTON2_GPIO_Port, .pin = BUTTON2_Pin },
    { .port = BUTTON3_GPIO_Port, .pin = BUTTON3_Pin },
};

static inline uint8_t key_read_pressed(const key_ctx_t *k)
{
    return (HAL_GPIO_ReadPin(k->port, k->pin) == USER_CONFIG_KEY_PRESSED_LEVEL) ? 1u : 0u;
}

static void key_push_event_from_isr(uint8_t key_id, key_event_type_t type)
{
    key_event_t evt;
    BaseType_t higher_woken = pdFALSE;

    if (keyEvtQueue == NULL) {
        return;
    }
    evt.key_id = key_id;
    evt.event_type = (uint8_t)type;
    evt.reserved = 0;
    evt.tick_ms = s_tick_ms;
    (void)xQueueSendFromISR(keyEvtQueue, &evt, &higher_woken);
    portYIELD_FROM_ISR(higher_woken);
}

static void key_on_stable_press(key_ctx_t *k)
{
    k->press_ms = 0;
    k->long_sent = 0;
}

static void key_on_stable_release(uint8_t key_id, key_ctx_t *k)
{
    if (k->long_sent) {
        k->click_cnt = 0;
        k->click_window = 0;
        return;
    }
    k->click_cnt++;
    if (k->click_cnt == 1u) {
        k->click_window = USER_CONFIG_KEY_DOUBLE_WINDOW_MS;
    } else {
        key_push_event_from_isr(key_id, KEY_EVENT_DOUBLE_CLICK);
        k->click_cnt = 0;
        k->click_window = 0;
    }
}

static void key_tick_1ms(uint8_t key_id, key_ctx_t *k)
{
    const uint8_t raw = key_read_pressed(k);

    if (raw != k->raw_last) {
        k->raw_last = raw;
        k->debounce_cnt = 0;
    } else if (k->debounce_cnt < USER_CONFIG_KEY_DEBOUNCE_MS) {
        k->debounce_cnt++;
        if (k->debounce_cnt == USER_CONFIG_KEY_DEBOUNCE_MS && k->stable != raw) {
            k->stable = raw;
            if (k->stable) {
                key_on_stable_press(k);
            } else {
                key_on_stable_release(key_id, k);
            }
        }
    }

    if (k->stable) {
        if (k->press_ms < 0xFFFFu) {
            k->press_ms++;
        }
        if (!k->long_sent && k->press_ms >= USER_CONFIG_KEY_LONG_PRESS_MS) {
            k->long_sent = 1;
            key_push_event_from_isr(key_id, KEY_EVENT_LONG_PRESS);
            k->click_cnt = 0;
            k->click_window = 0;
        }
    }

    if (k->click_window > 0u) {
        k->click_window--;
        if (k->click_window == 0u && k->click_cnt == 1u) {
            key_push_event_from_isr(key_id, KEY_EVENT_SINGLE_CLICK);
            k->click_cnt = 0;
        }
    }
}

void Key_Task_1msTickISR(void)
{
    s_tick_ms++;
    key_tick_1ms((uint8_t)KEY_ID_1, &s_keys[0]);
    key_tick_1ms((uint8_t)KEY_ID_2, &s_keys[1]);
    key_tick_1ms((uint8_t)KEY_ID_3, &s_keys[2]);
}

static void Key_Task(void *argument)
{
    key_event_t evt;
    (void)argument;

    Log_Print(LOG_LEVEL_INFO, "[Key] started");
    for (;;) {
        if (xQueueReceive(keyEvtQueue, &evt, portMAX_DELAY) == pdTRUE) {
            for (uint32_t i = 0; i < KEY_CB_MAX; i++) {
                if (s_cbs[i] != NULL) {
                    s_cbs[i](&evt, s_cb_users[i]);
                }
            }
        }
    }
}

bool Key_RegisterCallback(key_event_cb_t cb, void *user)
{
    taskENTER_CRITICAL();
    for (uint32_t i = 0; i < KEY_CB_MAX; i++) {
        if (s_cbs[i] == NULL) {
            s_cbs[i] = cb;
            s_cb_users[i] = user;
            taskEXIT_CRITICAL();
            return true;
        }
    }
    taskEXIT_CRITICAL();
    return false;
}

bool Key_Task_Create(void)
{
    const osThreadAttr_t attr = {
        .name = "Key_Task",
        .cb_mem = &keyTaskTCB,
        .cb_size = sizeof(keyTaskTCB),
        .stack_mem = keyTaskStack,
        .stack_size = sizeof(keyTaskStack),
        .priority = (osPriority_t)USER_CONFIG_KEY_TASK_PRIORITY,
    };

    if (keyTaskHandle != NULL) {
        return true;
    }
    keyEvtQueue = xQueueCreateStatic(USER_CONFIG_KEY_EVT_QUEUE_LEN,
                                     sizeof(key_event_t),
                                     keyEvtQueueStorage,
                                     &keyEvtQueueCtrl);
    if (keyEvtQueue == NULL) {
        return false;
    }
    keyTaskHandle = osThreadNew(Key_Task, NULL, &attr);
    return (keyTaskHandle != NULL);
}
