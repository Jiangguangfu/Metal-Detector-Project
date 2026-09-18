#include "cdc_task.h"
#include "user_config.h"
#include "adc_task.h"
#include "led_task.h"
#include "buzzer_task.h"
#include "pulse_capture.h"
#include "usbd_cdc_if.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>

typedef struct {
    uint16_t len;
    uint8_t data[64];
} cdc_tx_item_t;

static osThreadId_t cdcTaskHandle;
static osThreadId_t cdcTxTaskHandle;
static StackType_t cdcTaskStack[USER_CONFIG_CDC_TASK_STACK_SIZE];
static StaticTask_t cdcTaskTCB;
static StackType_t cdcTxTaskStack[USER_CONFIG_CDC_TX_TASK_STACK_SIZE];
static StaticTask_t cdcTxTaskTCB;
static volatile bool cdc_usb_connected;
static uint8_t rx_buf[USER_CONFIG_CDC_RX_BUFFER_SIZE];
static volatile uint32_t rx_w;
static volatile uint32_t rx_r;
static char cmd_line[80];
static uint8_t cmd_len;
static QueueHandle_t tx_queue;
static uint8_t tx_storage[USER_CONFIG_CDC_TX_QUEUE_SIZE * sizeof(cdc_tx_item_t)];
static StaticQueue_t tx_queue_ctrl;

bool CDC_Task_IsUSBConnected(void)
{
    return cdc_usb_connected;
}

void CDC_Task_SetUSBConnected(bool connected)
{
    cdc_usb_connected = connected;
    if (connected) {
        rx_w = 0;
        rx_r = 0;
        cmd_len = 0;
    }
}

void CDC_Task_ReceiveData(const uint8_t *data, uint16_t len)
{
    uint32_t w;
    uint16_t i;

    if (data == NULL || len == 0u || !cdc_usb_connected) {
        return;
    }
    w = rx_w;
    for (i = 0; i < len; i++) {
        uint32_t next = (w + 1u) % USER_CONFIG_CDC_RX_BUFFER_SIZE;
        if (next == rx_r) {
            break;
        }
        rx_buf[w] = data[i];
        w = next;
    }
    __DMB();
    rx_w = w;
}

bool CDC_Task_Send(const uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    cdc_tx_item_t item;
    if (data == NULL || len == 0u || !cdc_usb_connected || tx_queue == NULL) {
        return false;
    }
    if (len > sizeof(item.data)) {
        len = sizeof(item.data);
    }
    item.len = len;
    memcpy(item.data, data, len);
    return xQueueSend(tx_queue, &item, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

bool CDC_Task_SendString(const char *str)
{
    if (str == NULL) {
        return false;
    }
    return CDC_Task_Send((const uint8_t *)str, (uint16_t)strlen(str), 30u);
}

static void cdc_tx_task(void *argument)
{
    cdc_tx_item_t item;
    (void)argument;

    for (;;) {
        if (xQueueReceive(tx_queue, &item, portMAX_DELAY) == pdTRUE) {
            uint32_t start = osKernelGetTickCount();
            while (cdc_usb_connected) {
                if (CDC_Transmit_FS(item.data, item.len) == USBD_OK) {
                    break;
                }
                if ((osKernelGetTickCount() - start) > 50u) {
                    break;
                }
                osDelay(1);
            }
        }
    }
}

static void cdc_help(void)
{
    CDC_Task_SendString("CMD: help|status|led1 on|led1 off|led2 on|led2 off|beep [hz] [ms]\r\n");
}

static void cdc_status(void)
{
    adc_data_t adc = {0};
    uint32_t frequency_hz = 0u;
    char line[96];
    (void)ADC_CopyLatest(&adc);
    (void)Pulse_Capture_GetFrequency(&frequency_hz);
    unsigned mv = (unsigned)(adc.voltage_power * 1000.0f + 0.5f);
    snprintf(line, sizeof(line),
             "USB=1 Vin=%umV LC=%luHz LED=%d%d n=%lu\r\n",
             mv,
             (unsigned long)frequency_hz,
             LED_GetGPIO(0) ? 1 : 0,
             LED_GetGPIO(1) ? 1 : 0,
             (unsigned long)adc.sample_count);
    CDC_Task_SendString(line);
}

static void cdc_handle_line(char *line)
{
    unsigned hz = USER_CONFIG_BUZZER_DEFAULT_HZ;
    unsigned ms = 80u;

    if (line == NULL || line[0] == '\0') {
        return;
    }
    if (strcmp(line, "help") == 0) {
        cdc_help();
    } else if (strcmp(line, "status") == 0) {
        cdc_status();
    } else if (strcmp(line, "led1 on") == 0) {
        LED_SetGPIO(0, true);
        CDC_Task_SendString("OK\r\n");
    } else if (strcmp(line, "led1 off") == 0) {
        LED_SetGPIO(0, false);
        CDC_Task_SendString("OK\r\n");
    } else if (strcmp(line, "led2 on") == 0) {
        LED_SetGPIO(1, true);
        CDC_Task_SendString("OK\r\n");
    } else if (strcmp(line, "led2 off") == 0) {
        LED_SetGPIO(1, false);
        CDC_Task_SendString("OK\r\n");
    } else if (strncmp(line, "beep", 4) == 0) {
        (void)sscanf(line + 4, "%u %u", &hz, &ms);
        if (hz == 0u) {
            hz = USER_CONFIG_BUZZER_DEFAULT_HZ;
        }
        if (ms == 0u) {
            ms = 80u;
        }
        (void)Buzzer_Beep((uint16_t)hz, (uint16_t)ms);
        CDC_Task_SendString("OK\r\n");
    } else {
        CDC_Task_SendString("ERR unknown. help\r\n");
    }
}

static void CDC_Task(void *argument)
{
    bool last_usb = false;
    (void)argument;

    tx_queue = xQueueCreateStatic(USER_CONFIG_CDC_TX_QUEUE_SIZE,
                                  sizeof(cdc_tx_item_t),
                                  tx_storage,
                                  &tx_queue_ctrl);

    {
        const osThreadAttr_t tx_attr = {
            .name = "cdc_tx",
            .cb_mem = &cdcTxTaskTCB,
            .cb_size = sizeof(cdcTxTaskTCB),
            .stack_mem = cdcTxTaskStack,
            .stack_size = sizeof(cdcTxTaskStack),
            .priority = (osPriority_t)USER_CONFIG_CDC_TX_TASK_PRIORITY,
        };
        cdcTxTaskHandle = osThreadNew(cdc_tx_task, NULL, &tx_attr);
        (void)cdcTxTaskHandle;
    }

    for (;;) {
        if (cdc_usb_connected != last_usb) {
            last_usb = cdc_usb_connected;
            if (last_usb) {
                osDelay(50);
                CDC_Task_SendString("\r\nMetal Detector CDC ready. Type help\r\n");
            }
        }

        while (rx_r != rx_w) {
            char ch = (char)rx_buf[rx_r];
            rx_r = (rx_r + 1u) % USER_CONFIG_CDC_RX_BUFFER_SIZE;
            if (ch == '\r' || ch == '\n') {
                if (cmd_len > 0u) {
                    cmd_line[cmd_len] = '\0';
                    cdc_handle_line(cmd_line);
                    cmd_len = 0;
                }
            } else if (cmd_len < (sizeof(cmd_line) - 1u)) {
                cmd_line[cmd_len++] = ch;
            }
        }
        osDelay(USER_CONFIG_CDC_TASK_PERIOD_MS);
    }
}

bool CDC_Task_Create(void)
{
    const osThreadAttr_t attr = {
        .name = "CDC_Task",
        .cb_mem = &cdcTaskTCB,
        .cb_size = sizeof(cdcTaskTCB),
        .stack_mem = cdcTaskStack,
        .stack_size = sizeof(cdcTaskStack),
        .priority = (osPriority_t)USER_CONFIG_CDC_TASK_PRIORITY,
    };

    if (cdcTaskHandle != NULL) {
        return true;
    }
    cdcTaskHandle = osThreadNew(CDC_Task, NULL, &attr);
    return (cdcTaskHandle != NULL);
}
