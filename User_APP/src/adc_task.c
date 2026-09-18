#include "adc_task.h"
#include "user_config.h"
#include "log_task.h"
#include "adc.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"

extern ADC_HandleTypeDef hadc1;

static osThreadId_t adcTaskHandle;
static StackType_t adcTaskStack[USER_CONFIG_ADC_TASK_STACK_SIZE];
static StaticTask_t adcTaskTCB;
static adc_data_t adc_data;
static bool adc_ready;
static float s_vdda = USER_CONFIG_ADC_VDDA_FALLBACK_V;
static float s_filt = -1.0f;

static uint16_t adc_read_channel(uint32_t channel, uint32_t sample_time)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    sConfig.Channel = channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = sample_time;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return 0xFFFFu;
    }
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return 0xFFFFu;
    }
    if (HAL_ADC_PollForConversion(&hadc1, 20) != HAL_OK) {
        (void)HAL_ADC_Stop(&hadc1);
        return 0xFFFFu;
    }
    {
        uint16_t v = (uint16_t)HAL_ADC_GetValue(&hadc1);
        (void)HAL_ADC_Stop(&hadc1);
        return v;
    }
}

static void adc_calibrate_vdda(void)
{
    uint16_t vref;
    hadc1.Instance->CR2 |= ADC_CR2_TSVREFE;
    osDelay(2);
    vref = adc_read_channel(ADC_CHANNEL_VREFINT, ADC_SAMPLETIME_239CYCLES_5);
    if (vref > 0u && vref < 0xFFFFu) {
        s_vdda = (USER_CONFIG_ADC_VREFINT_V * 4095.0f) / (float)vref;
    } else {
        s_vdda = USER_CONFIG_ADC_VDDA_FALLBACK_V;
    }
}

static float adc_lpf(float in, float *prev, float alpha)
{
    if (*prev < 0.0f) {
        *prev = in;
        return in;
    }
    *prev = alpha * in + (1.0f - alpha) * (*prev);
    return *prev;
}

static void ADC_Task(void *argument)
{
    const float rtop = USER_CONFIG_ADC_DIV_RTOP_OHM;
    const float rbot = USER_CONFIG_ADC_DIV_RBOT_OHM;
    const float div = (rtop + rbot) > 0.0f ? (rbot / (rtop + rbot)) : 1.0f;
    (void)argument;

    osDelay(20);
    HAL_ADCEx_Calibration_Start(&hadc1);
    adc_calibrate_vdda();
    Log_Printf(LOG_LEVEL_INFO, "[ADC] VDDA=%umV",
               (unsigned)(s_vdda * 1000.0f + 0.5f));

    for (;;) {
        uint16_t raw = adc_read_channel(ADC_CHANNEL_5, ADC_SAMPLETIME_55CYCLES_5);
        float vin_adc;
        float vpack;
        float filt;

        if (raw == 0xFFFFu) {
            osDelay(USER_CONFIG_ADC_MEASURE_PERIOD_MS);
            continue;
        }

        vin_adc = ((float)raw * s_vdda) / 4095.0f;
        vpack = vin_adc / div;
        filt = adc_lpf(vpack, &s_filt, USER_CONFIG_ADC_LPF_ALPHA);

        taskENTER_CRITICAL();
        adc_data.raw_power = raw;
        adc_data.voltage_power = filt;
        adc_data.vdda = s_vdda;
        adc_data.data_ready = true;
        adc_data.sample_count++;
        adc_ready = true;
        taskEXIT_CRITICAL();

        osDelay(USER_CONFIG_ADC_MEASURE_PERIOD_MS);
    }
}

void ADC_Task_Create(void)
{
    const osThreadAttr_t attr = {
        .name = "ADC_Task",
        .cb_mem = &adcTaskTCB,
        .cb_size = sizeof(adcTaskTCB),
        .stack_mem = adcTaskStack,
        .stack_size = sizeof(adcTaskStack),
        .priority = (osPriority_t)USER_CONFIG_ADC_TASK_PRIORITY,
    };

    if (adcTaskHandle != NULL) {
        return;
    }
    adcTaskHandle = osThreadNew(ADC_Task, NULL, &attr);
}

bool ADC_GetData(adc_data_t *data)
{
    bool ready;
    if (data == NULL || !adc_ready) {
        return false;
    }
    taskENTER_CRITICAL();
    *data = adc_data;
    ready = adc_data.data_ready;
    adc_data.data_ready = false;
    taskEXIT_CRITICAL();
    return ready;
}

bool ADC_CopyLatest(adc_data_t *data)
{
    if (data == NULL || !adc_ready) {
        return false;
    }
    taskENTER_CRITICAL();
    *data = adc_data;
    taskEXIT_CRITICAL();
    return true;
}
