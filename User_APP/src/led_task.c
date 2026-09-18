#include "led_task.h"
#include "user_config.h"
#include "cdc_task.h"
#include "main.h"
#include "log_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

static osThreadId_t ledTaskHandle;
static osMutexId_t ledMutex;
static StackType_t ledTaskStack[USER_CONFIG_LED_TASK_STACK_SIZE];
static StaticTask_t ledTaskTCB;
static bool gpio_led[GPIO_LED_COUNT];
static bool led1_manual;
static bool led2_manual;

static void gpio_led_apply(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin,
                      gpio_led[LED_INDEX_LED1] ? USER_CONFIG_LED_ON_LEVEL
                                               : (GPIO_PinState)!USER_CONFIG_LED_ON_LEVEL);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin,
                      gpio_led[LED_INDEX_LED2] ? USER_CONFIG_LED_ON_LEVEL
                                               : (GPIO_PinState)!USER_CONFIG_LED_ON_LEVEL);
}

static void LED_Task(void *argument)
{
    uint32_t last_hb = 0;
    (void)argument;

    Log_Print(LOG_LEVEL_INFO, "[LED] started");
    gpio_led_apply();

    for (;;) {
        uint32_t now = osKernelGetTickCount();
        if (osMutexAcquire(ledMutex, 10) == osOK) {
            if (!led1_manual && (now - last_hb) >= USER_CONFIG_LED_HEARTBEAT_MS) {
                gpio_led[LED_INDEX_LED1] = !gpio_led[LED_INDEX_LED1];
                last_hb = now;
            }
            if (!led2_manual) {
                gpio_led[LED_INDEX_LED2] = CDC_Task_IsUSBConnected();
            }
            gpio_led_apply();
            osMutexRelease(ledMutex);
        }
        osDelay(20);
    }
}

void LED_Task_Create(void)
{
    static const osMutexAttr_t mutex_attr = { .name = "LED_Mutex" };
    const osThreadAttr_t attr = {
        .name = "LED_Task",
        .cb_mem = &ledTaskTCB,
        .cb_size = sizeof(ledTaskTCB),
        .stack_mem = ledTaskStack,
        .stack_size = sizeof(ledTaskStack),
        .priority = (osPriority_t)USER_CONFIG_LED_TASK_PRIORITY,
    };

    if (ledTaskHandle != NULL) {
        return;
    }
    ledMutex = osMutexNew(&mutex_attr);
    ledTaskHandle = osThreadNew(LED_Task, NULL, &attr);
}

int LED_SetGPIO(uint8_t index, bool state)
{
    if (index >= GPIO_LED_COUNT) {
        return -1;
    }
    if (ledMutex == NULL || osMutexAcquire(ledMutex, 50) != osOK) {
        return -1;
    }
    gpio_led[index] = state;
    if (index == LED_INDEX_LED1) {
        led1_manual = true;
    } else if (index == LED_INDEX_LED2) {
        led2_manual = true;
    }
    gpio_led_apply();
    osMutexRelease(ledMutex);
    return 0;
}

bool LED_GetGPIO(uint8_t index)
{
    bool state = false;
    if (index >= GPIO_LED_COUNT) {
        return false;
    }
    if (ledMutex != NULL && osMutexAcquire(ledMutex, 10) == osOK) {
        state = gpio_led[index];
        osMutexRelease(ledMutex);
    }
    return state;
}

void LED_ToggleGPIO(uint8_t index)
{
    (void)LED_SetGPIO(index, !LED_GetGPIO(index));
}
