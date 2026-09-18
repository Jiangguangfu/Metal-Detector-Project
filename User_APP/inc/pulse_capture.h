#ifndef PULSE_CAPTURE_H
#define PULSE_CAPTURE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

bool Pulse_Capture_Start(void);
bool Pulse_Capture_GetFrequency(uint32_t *frequency_hz);

#ifdef __cplusplus
}
#endif

#endif /* PULSE_CAPTURE_H */
