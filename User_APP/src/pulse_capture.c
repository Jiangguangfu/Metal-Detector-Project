#include "pulse_capture.h"
#include "main.h"
#include "tim.h"
#include "stm32f1xx_hal.h"

extern TIM_HandleTypeDef htim1;

#define TIM1_COUNTER_HZ          1000000u   /* 与蜂鸣器 PWM 相同的 1 MHz 时基 */
#define TIM1_IC_EDGE_DIVIDER     8u         /* CubeMX：每 8 个上升沿捕获一次 */
#define CAPTURE_AVERAGE_SAMPLES  8u         /* 再对 8 次间隔求平均 */
#define CAPTURE_STALE_MS         500u       /* 超时认为探头无信号 */

static volatile uint16_t s_previous_capture;
static volatile uint32_t s_delta_sum;
static volatile uint32_t s_frequency_hz;
static volatile uint32_t s_last_update_ms;
static volatile uint8_t s_delta_count;
static volatile bool s_have_previous;
static volatile bool s_running;

bool Pulse_Capture_Start(void)
{
    if (s_running) {
        return true;
    }

    s_have_previous = false;
    s_delta_sum = 0u;
    s_delta_count = 0u;
    s_frequency_hz = 0u;
    s_last_update_ms = HAL_GetTick();

    if (HAL_TIM_IC_Start_IT(&htim1, TIM_CHANNEL_2) != HAL_OK) {
        return false;
    }
    s_running = true;
    return true;
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    uint16_t capture;
    uint16_t delta;

    if (htim == NULL || htim->Instance != TIM1 ||
        htim->Channel != HAL_TIM_ACTIVE_CHANNEL_2) {
        return;
    }

    capture = (uint16_t)HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
    if (!s_have_previous) {
        s_previous_capture = capture;
        s_have_previous = true;
        return;
    }

    /* 16 位计数器自然回绕，相减即可得到间隔 */
    delta = (uint16_t)(capture - s_previous_capture);
    s_previous_capture = capture;
    if (delta == 0u) {
        return;
    }

    s_delta_sum += delta;
    s_delta_count++;
    if (s_delta_count >= CAPTURE_AVERAGE_SAMPLES) {
        /* f = 1 MHz * 8边沿分频 * 8次平均 / ΣΔCNT */
        const uint32_t numerator =
            TIM1_COUNTER_HZ * TIM1_IC_EDGE_DIVIDER * CAPTURE_AVERAGE_SAMPLES;
        s_frequency_hz = numerator / s_delta_sum;
        s_delta_sum = 0u;
        s_delta_count = 0u;
        s_last_update_ms = HAL_GetTick();
    }
}

bool Pulse_Capture_GetFrequency(uint32_t *frequency_hz)
{
    uint32_t frequency;
    uint32_t updated;
    uint32_t primask;

    if (frequency_hz == NULL || !s_running) {
        return false;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    frequency = s_frequency_hz;
    updated = s_last_update_ms;
    if (primask == 0u) {
        __enable_irq();
    }

    if (frequency == 0u || (HAL_GetTick() - updated) > CAPTURE_STALE_MS) {
        *frequency_hz = 0u;
        return false;
    }

    *frequency_hz = frequency;
    return true;
}
