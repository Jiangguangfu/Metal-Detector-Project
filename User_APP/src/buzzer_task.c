#include "buzzer_task.h"
#include "user_config.h"
#include "log_task.h"
#include "tim.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

extern TIM_HandleTypeDef htim1;

#define TIM1_TICK_HZ 1000000u
#define BUZZ_NOTIFY_BEEP  (1u << 0)
#define BUZZ_NOTIFY_PLAY  (1u << 1)
#define BUZZ_NOTIFY_STOP  (1u << 2)
#define BUZZ_NOTIFY_BOOT  (1u << 3)
#define BUZZ_MAX_NOTES    16u

static osThreadId_t buzzerTaskHandle;
static StackType_t buzzerTaskStack[USER_CONFIG_BUZZER_TASK_STACK_SIZE];
static StaticTask_t buzzerTaskTCB;
static buzzer_note_t s_pending_beep;
static buzzer_note_t s_song[BUZZ_MAX_NOTES];
static volatile uint16_t s_song_count;
static volatile uint16_t s_buzzer_half_period_ticks;
static volatile bool s_buzzer_active;

static bool buzzer_oc_prepare(void)
{
    /* PSC/ARR are fixed by MX_TIM1_Init because CH1 and CH2 share CNT. */
    MODIFY_REG(htim1.Instance->CCMR1, TIM_CCMR1_OC1M, TIM_OCMODE_FORCED_INACTIVE);
    SET_BIT(htim1.Instance->CCER, TIM_CCER_CC1E);
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_ENABLE(&htim1);
    return true;
}

static void Buzzer_PWM_SetFreq(uint16_t freq_hz, uint8_t volume_percent)
{
    uint32_t half_period;
    (void)volume_percent; /* Output-compare toggle produces a fixed 50% duty cycle. */

    if (freq_hz == 0u) {
        taskENTER_CRITICAL();
        s_buzzer_active = false;
        CLEAR_BIT(htim1.Instance->DIER, TIM_DIER_CC1IE);
        CLEAR_BIT(htim1.Instance->CCER, TIM_CCER_CC1E);
        MODIFY_REG(htim1.Instance->CCMR1, TIM_CCMR1_OC1M, TIM_OCMODE_FORCED_INACTIVE);
        SET_BIT(htim1.Instance->CCER, TIM_CCER_CC1E);
        taskEXIT_CRITICAL();
        return;
    }

    half_period = TIM1_TICK_HZ / (2u * (uint32_t)freq_hz);
    if (half_period == 0u) {
        half_period = 1u;
    }
    if (half_period > 0xFFFFu) {
        half_period = 0xFFFFu;
    }

    taskENTER_CRITICAL();
    s_buzzer_half_period_ticks = (uint16_t)half_period;
    CLEAR_BIT(htim1.Instance->CCER, TIM_CCER_CC1E);
    MODIFY_REG(htim1.Instance->CCMR1, TIM_CCMR1_OC1M, TIM_OCMODE_TOGGLE);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,
                          (uint16_t)(__HAL_TIM_GET_COUNTER(&htim1) + half_period));
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_CC1);
    SET_BIT(htim1.Instance->DIER, TIM_DIER_CC1IE);
    SET_BIT(htim1.Instance->CCER, TIM_CCER_CC1E);
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_ENABLE(&htim1);
    s_buzzer_active = true;
    taskEXIT_CRITICAL();
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim != NULL && htim->Instance == TIM1 &&
        htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1 && s_buzzer_active) {
        uint16_t next = (uint16_t)(__HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1) +
                                   s_buzzer_half_period_ticks);
        __HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1, next);
    }
}

static void buzzer_play_sequence(const buzzer_note_t *notes, uint16_t count, uint8_t volume)
{
    uint16_t i;
    if (notes == NULL || count == 0u) {
        return;
    }
    for (i = 0; i < count; i++) {
        Buzzer_PWM_SetFreq(notes[i].freq_hz, volume);
        osDelay(notes[i].dur_ms);
        Buzzer_PWM_SetFreq(0u, 0u);
        osDelay(20);
    }
}

/* PawDrive BatAlert：500Hz → 1kHz，100 步 × 10ms ≈ 1s。
 * 共用计数器后由输出比较翻转产生固定 50% 占空比。 */
static void buzzer_boot_melody(void)
{
    const uint32_t f0 = 500u;
    const uint32_t f1 = 1000u;
    const uint32_t total_ms = 1000u;
    const uint32_t step_ms = 10u;
    const uint32_t steps = total_ms / step_ms;
    uint32_t i;

    for (i = 0u; i < steps; i++) {
        uint32_t f = f0 + (((f1 - f0) * i) / (steps - 1u));
        if (f > 65000u) {
            f = 65000u;
        }
        Buzzer_PWM_SetFreq((uint16_t)f, USER_CONFIG_BUZZER_BOOT_VOLUME_PERCENT);
        osDelay(step_ms);
    }
    Buzzer_PWM_SetFreq(0u, 0u);
}

static void Buzzer_Task(void *argument)
{
    (void)argument;

    if (!buzzer_oc_prepare()) {
        Log_Print(LOG_LEVEL_ERROR, "[Buzzer] TIM1 OC prepare failed");
        vTaskDelete(NULL);
        return;
    }
    Log_Print(LOG_LEVEL_INFO, "[Buzzer] TIM1_CH1 ready");

#if USER_CONFIG_BUZZER_BOOT_SONG_ENABLE
    Log_Print(LOG_LEVEL_INFO, "[Buzzer] startup: delay 2s then sweep 500Hz->1kHz @1s (TIM1_CH1)");
    osDelay(USER_CONFIG_BUZZER_BOOT_DELAY_MS);
    buzzer_boot_melody();
#else
    Log_Print(LOG_LEVEL_INFO, "[Buzzer] startup melody disabled");
#endif

    for (;;) {
        uint32_t notify = 0u;
        (void)xTaskNotifyWait(0u, 0xFFFFFFFFu, &notify, portMAX_DELAY);

        if ((notify & BUZZ_NOTIFY_STOP) != 0u) {
            Buzzer_PWM_SetFreq(0u, 0u);
        }
        if ((notify & BUZZ_NOTIFY_BEEP) != 0u) {
            buzzer_play_sequence(&s_pending_beep, 1u, 40u);
        }
        if ((notify & BUZZ_NOTIFY_PLAY) != 0u) {
            buzzer_play_sequence(s_song, s_song_count, USER_CONFIG_BUZZER_VOLUME_PERCENT);
        }
        if ((notify & BUZZ_NOTIFY_BOOT) != 0u) {
            buzzer_boot_melody();
        }
    }
}

bool Buzzer_Task_Create(void)
{
    const osThreadAttr_t attr = {
        .name = "Buzzer",
        .cb_mem = &buzzerTaskTCB,
        .cb_size = sizeof(buzzerTaskTCB),
        .stack_mem = buzzerTaskStack,
        .stack_size = sizeof(buzzerTaskStack),
        .priority = (osPriority_t)USER_CONFIG_BUZZER_TASK_PRIORITY,
    };

    if (buzzerTaskHandle != NULL) {
        return true;
    }
    buzzerTaskHandle = osThreadNew(Buzzer_Task, NULL, &attr);
    return (buzzerTaskHandle != NULL);
}

bool Buzzer_Beep(uint16_t freq_hz, uint16_t dur_ms)
{
    if (buzzerTaskHandle == NULL) {
        return false;
    }
    s_pending_beep.freq_hz = freq_hz;
    s_pending_beep.dur_ms = dur_ms;
    (void)xTaskNotify(buzzerTaskHandle, BUZZ_NOTIFY_BEEP, eSetBits);
    return true;
}

bool Buzzer_Play(const buzzer_note_t *notes, uint16_t count)
{
    uint16_t n = count;
    if (buzzerTaskHandle == NULL || notes == NULL || count == 0u) {
        return false;
    }
    if (n > BUZZ_MAX_NOTES) {
        n = BUZZ_MAX_NOTES;
    }
    memcpy(s_song, notes, n * sizeof(buzzer_note_t));
    s_song_count = n;
    (void)xTaskNotify(buzzerTaskHandle, BUZZ_NOTIFY_PLAY, eSetBits);
    return true;
}

void Buzzer_Stop(void)
{
    if (buzzerTaskHandle != NULL) {
        (void)xTaskNotify(buzzerTaskHandle, BUZZ_NOTIFY_STOP, eSetBits);
    }
}
