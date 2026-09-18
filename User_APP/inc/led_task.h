#ifndef LED_TASK_H
#define LED_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define LED_INDEX_LED1  0   /* PA3 */
#define LED_INDEX_LED2  1   /* PA4 */
#define GPIO_LED_COUNT  2

void LED_Task_Create(void);
/* state=true 点亮。手动设置后该灯不再由心跳/USB 自动控制 */
int LED_SetGPIO(uint8_t index, bool state);
bool LED_GetGPIO(uint8_t index);
void LED_ToggleGPIO(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif /* LED_TASK_H */
