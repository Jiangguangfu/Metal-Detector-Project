#include "lcd_pages.h"
#include "lcd_priv.h"
#include "adc_task.h"
#include "cdc_task.h"
#include "key_task.h"
#include "led_task.h"
#include "pulse_capture.h"
#include "cmsis_os2.h"
#include <stdio.h>

static uint8_t s_last_key_id;
static uint8_t s_last_key_event;
static uint32_t s_last_key_tick;

void lcd_page_home_handle_key(uint8_t key_id, uint8_t event_type)
{
    s_last_key_id = key_id;
    s_last_key_event = event_type;
    s_last_key_tick = osKernelGetTickCount();
}

/* 128x32 共 4 行：标题 / 电压+USB / LC+LED / 按键 */
void lcd_page_home_render(void)
{
    adc_data_t adc = {0};
    uint32_t frequency_hz = 0u;
    char line[22];
    const char *key_evt = "-";

    (void)ADC_CopyLatest(&adc);

    LCD_PrintString_Internal(0, 0, "Metal Detector", FONT_SIZE_8X8);

    if (adc.sample_count > 0u) {
        unsigned mv = (unsigned)(adc.voltage_power * 1000.0f + 0.5f);
        snprintf(line, sizeof(line), "Vin %5u USB %s",
                 mv, CDC_Task_IsUSBConnected() ? "ON " : "OFF");
    } else {
        snprintf(line, sizeof(line), "Vin  ---- USB %s",
                 CDC_Task_IsUSBConnected() ? "ON " : "OFF");
    }
    LCD_PrintString_Internal(0, 1, line, FONT_SIZE_6X8);

    if (Pulse_Capture_GetFrequency(&frequency_hz)) {
        snprintf(line, sizeof(line), "LC %luHz LED %d%d",
                 (unsigned long)frequency_hz,
                 LED_GetGPIO(LED_INDEX_LED1) ? 1 : 0,
                 LED_GetGPIO(LED_INDEX_LED2) ? 1 : 0);
    } else {
        snprintf(line, sizeof(line), "LC ----Hz LED %d%d",
                 LED_GetGPIO(LED_INDEX_LED1) ? 1 : 0,
                 LED_GetGPIO(LED_INDEX_LED2) ? 1 : 0);
    }
    LCD_PrintString_Internal(0, 2, line, FONT_SIZE_6X8);

    switch (s_last_key_event) {
    case KEY_EVENT_SINGLE_CLICK:
        key_evt = "click";
        break;
    case KEY_EVENT_DOUBLE_CLICK:
        key_evt = "dbl";
        break;
    case KEY_EVENT_LONG_PRESS:
        key_evt = "long";
        break;
    default:
        break;
    }
    if (s_last_key_id == 0u) {
        snprintf(line, sizeof(line), "Key --  t=%lu",
                 (unsigned long)osKernelGetTickCount());
    } else {
        snprintf(line, sizeof(line), "K%u %s t=%lu",
                 (unsigned)s_last_key_id, key_evt,
                 (unsigned long)osKernelGetTickCount());
    }
    LCD_PrintString_Internal(0, 3, line, FONT_SIZE_6X8);

    (void)s_last_key_tick;
}
