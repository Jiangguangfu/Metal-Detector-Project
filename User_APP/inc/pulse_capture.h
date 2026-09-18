#ifndef PULSE_CAPTURE_H
#define PULSE_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* 启动 TIM1_CH2 输入捕获，测量 LC 探头方波频率 */
bool Pulse_Capture_Start(void);
/* 读最近一次有效频率；超过 500 ms 无更新则返回 false，*frequency_hz=0 */
bool Pulse_Capture_GetFrequency(uint32_t *frequency_hz);

#ifdef __cplusplus
}
#endif

#endif /* PULSE_CAPTURE_H */
