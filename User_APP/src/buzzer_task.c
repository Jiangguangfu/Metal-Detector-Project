#include "buzzer_task.h"
#include "user_config.h"
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

static bool buzzer_pwm_prepare(void)
{
    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
        return false;
    }
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    return true;
}

/* Same formula as PawDrive TIM3 PWM: f = 1 MHz / (ARR + 1). PSC stays 71 so CH2 capture keeps a 1 MHz tick. */
static void Buzzer_PWM_SetFreq(uint16_t freq_hz, uint8_t volume_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (freq_hz == 0u) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_AUTORELOAD(&htim1, 0xFFFFu);
        return;
    }
    if (volume_percent > 100u) {
        volume_percent = 100u;
    }

    arr = TIM1_TICK_HZ / (uint32_t)freq_hz;
    if (arr == 0u) {
        arr = 1u;
    }
    arr -= 1u;
    if (arr > 0xFFFFu) {
        arr = 0xFFFFu;
    }

    __HAL_TIM_DISABLE(&htim1);
    __HAL_TIM_SET_AUTORELOAD(&htim1, arr);
    ccr = ((arr + 1u) * (uint32_t)volume_percent) / 100u;
    if (ccr > arr) {
        ccr = arr;
    }
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ccr);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_ENABLE(&htim1);
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

/* PawDrive BatAlert：500Hz → 1kHz，100 步 × 10ms ≈ 1s，占空比 40%。 */
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

    if (!buzzer_pwm_prepare()) {
        vTaskDelete(NULL);
        return;
    }

#if USER_CONFIG_BUZZER_BOOT_SONG_ENABLE
    osDelay(USER_CONFIG_BUZZER_BOOT_DELAY_MS);
    buzzer_boot_melody();
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
