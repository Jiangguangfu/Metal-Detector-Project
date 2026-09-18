#include "user_main.h"
#include "user_config.h"
#include "key_task.h"
#include "led_task.h"
#include "adc_task.h"
#include "lcd_task.h"
#include "cdc_task.h"
#include "buzzer_task.h"
#include "pulse_capture.h"
#include "main.h"
#include "cmsis_os2.h"

/* 按键业务：K1 短按蜂鸣、长按旋律；K2/K3 切换 LED */
static void user_on_key(const key_event_t *evt, void *user)
{
    (void)user;
    if (evt == NULL) {
        return;
    }

    if (evt->event_type == KEY_EVENT_SINGLE_CLICK) {
        if (evt->key_id == KEY_ID_1) {
            (void)Buzzer_Beep(USER_CONFIG_BUZZER_DEFAULT_HZ, 60u);
        } else if (evt->key_id == KEY_ID_2) {
            LED_ToggleGPIO(LED_INDEX_LED1);
        } else if (evt->key_id == KEY_ID_3) {
            LED_ToggleGPIO(LED_INDEX_LED2);
        }
    } else if (evt->event_type == KEY_EVENT_LONG_PRESS && evt->key_id == KEY_ID_1) {
        static const buzzer_note_t boot[] = {
            { 880, 80 }, { 1175, 80 }, { 1568, 120 },
        };
        (void)Buzzer_Play(boot, 3u);
    }
}

void user_main(void)
{
    (void)Key_Task_Create();
    (void)Key_RegisterCallback(user_on_key, NULL);
    LED_Task_Create();
    ADC_Task_Create();
    (void)Buzzer_Task_Create();
    (void)Pulse_Capture_Start();   /* TIM1_CH2 捕获 LC 频率，与蜂鸣器共用 TIM1 */
    (void)CDC_Task_Create();
    LCD_Task_Create();
}
