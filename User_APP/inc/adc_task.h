#ifndef ADC_TASK_H
#define ADC_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float voltage_power;
    uint16_t raw_power;
    float vdda;
    bool data_ready;
    uint32_t sample_count;
} adc_data_t;

void ADC_Task_Create(void);
bool ADC_GetData(adc_data_t *data);
bool ADC_CopyLatest(adc_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ADC_TASK_H */
