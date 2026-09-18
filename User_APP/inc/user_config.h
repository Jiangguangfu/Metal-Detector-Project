#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "cmsis_os2.h"
#include "stm32f1xx_hal.h"

/* -------------------- 按键 SW1/2/3 → PA0/PA1/PA2 -------------------- */
#define USER_CONFIG_KEY_TASK_STACK_SIZE         96u     /* 静态任务栈（字） */
#define USER_CONFIG_KEY_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_KEY_EVT_QUEUE_LEN           8u      /* 按键事件队列深度 */
#define USER_CONFIG_KEY_DEBOUNCE_MS             20u     /* 消抖时间 */
#define USER_CONFIG_KEY_DOUBLE_WINDOW_MS        250u    /* 双击判定窗口 */
#define USER_CONFIG_KEY_LONG_PRESS_MS           800u    /* 长按判定时间 */
/* 板级：外部下拉，按下接到高电平 */
#define USER_CONFIG_KEY_PRESSED_LEVEL           GPIO_PIN_SET

/* -------------------- LED1/LED2 → PA3/PA4 -------------------- */
#define USER_CONFIG_LED_TASK_STACK_SIZE         96u
#define USER_CONFIG_LED_TASK_PRIORITY           osPriorityLow
#define USER_CONFIG_LED_HEARTBEAT_MS            500u    /* LED1 心跳周期 */
/* 原理图：3.3V -> LED -> 10k -> PAx，低电平点亮 */
#define USER_CONFIG_LED_ON_LEVEL                GPIO_PIN_RESET

/* -------------------- ADC 电压采样 POWER_ADC → PA5 -------------------- */
#define USER_CONFIG_ADC_TASK_STACK_SIZE         128u
#define USER_CONFIG_ADC_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_ADC_MEASURE_PERIOD_MS       50u     /* 采样周期 */
#define USER_CONFIG_ADC_VDDA_FALLBACK_V         3.3f    /* VrefINT 失败时的 VDDA 默认值 */
#define USER_CONFIG_ADC_VREFINT_V               1.20f   /* F103 内部基准约 1.20 V */
/* PA5：R32=0Ω 串联，R31=10k 对地，当前 1:1 直测 POWER_IN。
 * POWER_IN 为 5V，F103 模拟脚不耐受 5V；若改 R32=10k 则变成 2:1。 */
#define USER_CONFIG_ADC_DIV_RTOP_OHM            0.0f
#define USER_CONFIG_ADC_DIV_RBOT_OHM            10000.0f
#define USER_CONFIG_ADC_LPF_ALPHA               0.15f   /* 一阶低通系数，越大越跟手 */

/* -------------------- 探头脉冲 TIM_IN PA9（LC + LM393 OUT_LC） -------------------- */
#define USER_CONFIG_PULSE_TASK_STACK_SIZE       96u
#define USER_CONFIG_PULSE_TASK_PRIORITY         osPriorityNormal
#define USER_CONFIG_PULSE_GATE_MS               100u
#define USER_CONFIG_PULSE_BASELINE_SAMPLES      10u
#define USER_CONFIG_PULSE_DETECT_HZ             80u     /* 相对基线的检出阈值（预留） */
#define USER_CONFIG_PULSE_BEEP_COOLDOWN_MS      250u
#define USER_CONFIG_PULSE_DETECT_BEEP           1

/* -------------------- OLED I2C SSD1306 0.91" 128x32 -------------------- */
#define USER_CONFIG_LCD_TASK_STACK_SIZE         160u
#define USER_CONFIG_LCD_TASK_PRIORITY           osPriorityLow
#define USER_CONFIG_LCD_REFRESH_PERIOD_MS       200u    /* 刷新周期 */
#define USER_CONFIG_LCD_I2C_ADDR_7BIT           0x3Cu   /* 常见地址；失败再试 0x3D */
#define USER_CONFIG_LCD_I2C_TIMEOUT_MS          50u
#define USER_CONFIG_LCD_ROTATE_180              0       /* 1 则画面旋转 180° */

/* -------------------- 蜂鸣器 TIM1_CH1 → PA8 -------------------- */
#define USER_CONFIG_BUZZER_TASK_STACK_SIZE      96u
#define USER_CONFIG_BUZZER_TASK_PRIORITY        osPriorityLow
#define USER_CONFIG_BUZZER_VOLUME_PERCENT       20u     /* 普通提示音占空比 */
/* 无源蜂鸣器谐振约 2.7 kHz（原理图 BUZZER1），Q1 S8050 高电平导通 */
#define USER_CONFIG_BUZZER_DEFAULT_HZ           2700u
/* 与 PawDrive BatAlert 开机音一致：延时后 500Hz→1kHz 扫频约 1s */
#define USER_CONFIG_BUZZER_BOOT_SONG_ENABLE     1
#define USER_CONFIG_BUZZER_BOOT_DELAY_MS        2000u
#define USER_CONFIG_BUZZER_BOOT_VOLUME_PERCENT  40u

/* -------------------- USB CDC 虚拟串口 -------------------- */
#define USER_CONFIG_CDC_TASK_STACK_SIZE         128u    /* 命令解析任务 */
#define USER_CONFIG_CDC_TX_TASK_STACK_SIZE      96u     /* 发送任务 */
#define USER_CONFIG_CDC_TASK_PRIORITY           osPriorityNormal
#define USER_CONFIG_CDC_TX_TASK_PRIORITY        osPriorityHigh
#define USER_CONFIG_CDC_TASK_PERIOD_MS          10u
#define USER_CONFIG_CDC_TX_QUEUE_SIZE           4u
#define USER_CONFIG_CDC_RX_BUFFER_SIZE          128u

#endif /* USER_CONFIG_H */
