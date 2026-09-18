#ifndef ADC_TASK_H
#define ADC_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float voltage_power;    /* 折算后的 POWER_IN 电压（V） */
    uint16_t raw_power;     /* ADC 原始值 0~4095 */
    float vdda;             /* 用 VrefINT 估出的 VDDA（V） */
    bool data_ready;
    uint32_t sample_count;
} adc_data_t;

void ADC_Task_Create(void);
/* 取走最新数据并清 data_ready */
bool ADC_GetData(adc_data_t *data);
/* 只拷贝，不改变 data_ready */
bool ADC_CopyLatest(adc_data_t *data);

#ifdef __cplusplus
}
#endif

#endif /* ADC_TASK_H */
