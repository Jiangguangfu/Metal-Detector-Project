#ifndef BUZZER_TASK_H
#define BUZZER_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t freq_hz;
    uint16_t dur_ms;
} buzzer_note_t;

bool Buzzer_Task_Create(void);
bool Buzzer_Beep(uint16_t freq_hz, uint16_t dur_ms);           /* 单音 */
bool Buzzer_Play(const buzzer_note_t *notes, uint16_t count);  /* 旋律 */
void Buzzer_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* BUZZER_TASK_H */
