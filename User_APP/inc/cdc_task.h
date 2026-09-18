#ifndef CDC_TASK_H
#define CDC_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

bool CDC_Task_Create(void);
bool CDC_Task_IsUSBConnected(void);
void CDC_Task_SetUSBConnected(bool connected);                 /* USB DTR/枚举状态 */
void CDC_Task_ReceiveData(const uint8_t *data, uint16_t len);  /* CDC 接收回调写入环形缓冲 */
bool CDC_Task_Send(const uint8_t *data, uint16_t len, uint32_t timeout_ms);
bool CDC_Task_SendString(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* CDC_TASK_H */
