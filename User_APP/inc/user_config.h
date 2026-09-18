#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"

/* -------------------- 按键 -------------------- */
#define USER_CONFIG_KEY_TASK_STACK_SIZE         96u
#define USER_CONFIG_KEY_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_KEY_EVT_QUEUE_LEN           8u
#define USER_CONFIG_KEY_DEBOUNCE_MS             20u
#define USER_CONFIG_KEY_DOUBLE_WINDOW_MS        250u
#define USER_CONFIG_KEY_LONG_PRESS_MS           800u
/* 板级按键：SW1/2/3 外部 10k 上拉 + 100nF，按下为低电平 */
#define USER_CONFIG_KEY_PRESSED_LEVEL           GPIO_PIN_RESET

/* -------------------- LED -------------------- */
#define USER_CONFIG_LED_TASK_STACK_SIZE         96u
#define USER_CONFIG_LED_TASK_PRIORITY           osPriorityLow
#define USER_CONFIG_LED_HEARTBEAT_MS            500u
/* 原理图：3.3V -> LED -> 10k -> PAx，低电平点亮 */
#define USER_CONFIG_LED_ON_LEVEL                GPIO_PIN_RESET

/* -------------------- ADC 电压采样 -------------------- */
#define USER_CONFIG_ADC_TASK_STACK_SIZE         128u
#define USER_CONFIG_ADC_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_ADC_MEASURE_PERIOD_MS       50u
#define USER_CONFIG_ADC_VDDA_FALLBACK_V         3.3f
#define USER_CONFIG_ADC_VREFINT_V               1.20f
/* PA5 POWER_ADC：R32=0Ω 串联，R31=10k 对地。当前为 1:1 直测 POWER_IN。
 * 原理图 POWER_IN 为 5V，F103 模拟脚不耐受 5V；若改 R32=10k 则变成 2:1。 */
#define USER_CONFIG_ADC_DIV_RTOP_OHM            0.0f
#define USER_CONFIG_ADC_DIV_RBOT_OHM            10000.0f
#define USER_CONFIG_ADC_LPF_ALPHA               0.15f

/* -------------------- 探头脉冲 TIM_IN PA9（LC + LM393 OUT_LC） -------------------- */
#define USER_CONFIG_PULSE_TASK_STACK_SIZE       96u
#define USER_CONFIG_PULSE_TASK_PRIORITY         osPriorityNormal
#define USER_CONFIG_PULSE_GATE_MS               100u
#define USER_CONFIG_PULSE_BASELINE_SAMPLES      10u
#define USER_CONFIG_PULSE_DETECT_HZ             80u
#define USER_CONFIG_PULSE_BEEP_COOLDOWN_MS      250u
#define USER_CONFIG_PULSE_DETECT_BEEP           1

/* -------------------- OLED I2C SSD1306 0.91" 128x32 -------------------- */
#define USER_CONFIG_LCD_TASK_STACK_SIZE         160u
#define USER_CONFIG_LCD_TASK_PRIORITY           osPriorityLow
#define USER_CONFIG_LCD_REFRESH_PERIOD_MS       200u
#define USER_CONFIG_LCD_I2C_ADDR_7BIT           0x3Cu
#define USER_CONFIG_LCD_I2C_TIMEOUT_MS          50u
#define USER_CONFIG_LCD_ROTATE_180              0

/* -------------------- 蜂鸣器 TIM1_CH1 -------------------- */
#define USER_CONFIG_BUZZER_TASK_STACK_SIZE      96u
#define USER_CONFIG_BUZZER_TASK_PRIORITY        osPriorityLow
#define USER_CONFIG_BUZZER_VOLUME_PERCENT       20u /* OC 翻转模式保留兼容，实际占空比 50% */
/* 无源蜂鸣器谐振约 2.7kHz（原理图 BUZZER1），Q1 S8050 高电平导通 */
#define USER_CONFIG_BUZZER_DEFAULT_HZ           2700u
/* 与 PawDrive BatAlert 开机音一致：延时后 500Hz→1kHz 扫频约 1s */
#define USER_CONFIG_BUZZER_BOOT_SONG_ENABLE     1
#define USER_CONFIG_BUZZER_BOOT_DELAY_MS        2000u
#define USER_CONFIG_BUZZER_BOOT_VOLUME_PERCENT  40u /* OC 翻转模式下实际占空比 50% */

/* -------------------- USB CDC -------------------- */
#define USER_CONFIG_CDC_TASK_STACK_SIZE         128u
#define USER_CONFIG_CDC_TX_TASK_STACK_SIZE      96u
#define USER_CONFIG_CDC_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_CDC_TX_TASK_PRIORITY        osPriorityHigh
#define USER_CONFIG_CDC_TASK_PERIOD_MS          10u
#define USER_CONFIG_CDC_TX_QUEUE_SIZE           4u
#define USER_CONFIG_CDC_RX_BUFFER_SIZE          128u

/* -------------------- 日志 -------------------- */
#define USER_CONFIG_LOG_TASK_STACK_SIZE         128u
#define USER_CONFIG_LOG_TASK_PRIORITY           osPriorityLow
#define USER_CONFIG_LOG_QUEUE_DEPTH             4u

#endif /* USER_CONFIG_H */
