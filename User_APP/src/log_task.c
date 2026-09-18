#include "log_task.h"
#include "user_config.h"
#include "cdc_task.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_MESSAGE_MAX_LEN 96

typedef struct {
    log_level_t level;
    char message[LOG_MESSAGE_MAX_LEN];
} log_message_t;

static osThreadId_t logTaskHandle;
static QueueHandle_t logQueue;
static uint8_t logQueueStorage[USER_CONFIG_LOG_QUEUE_DEPTH * sizeof(log_message_t)];
static StaticQueue_t logQueueCtrl;
static StackType_t logTaskStack[USER_CONFIG_LOG_TASK_STACK_SIZE];
static StaticTask_t logTaskTCB;

static const char *log_level_tag(log_level_t level)
{
    switch (level) {
    case LOG_LEVEL_ERROR:
        return "E";
    case LOG_LEVEL_WARN:
        return "W";
    case LOG_LEVEL_INFO:
        return "I";
    default:
        return "D";
    }
}

static void log_emit(const log_message_t *msg)
{
    char line[128];
    int n;

    if (msg == NULL) {
        return;
    }
    n = snprintf(line, sizeof(line), "[%s] %s\r\n", log_level_tag(msg->level), msg->message);
    if (n <= 0) {
        return;
    }
    (void)CDC_Task_Send((const uint8_t *)line, (uint16_t)n, 20u);
}

static void Log_Task(void *argument)
{
    log_message_t msg;
    (void)argument;

    for (;;) {
        if (xQueueReceive(logQueue, &msg, portMAX_DELAY) == pdTRUE) {
            log_emit(&msg);
        }
    }
}

void Log_Task_Create(void)
{
    const osThreadAttr_t attr = {
        .name = "Log_Task",
        .cb_mem = &logTaskTCB,
        .cb_size = sizeof(logTaskTCB),
        .stack_mem = logTaskStack,
        .stack_size = sizeof(logTaskStack),
        .priority = (osPriority_t)USER_CONFIG_LOG_TASK_PRIORITY,
    };

    if (logTaskHandle != NULL) {
        return;
    }
    logQueue = xQueueCreateStatic(USER_CONFIG_LOG_QUEUE_DEPTH,
                                  sizeof(log_message_t),
                                  logQueueStorage,
                                  &logQueueCtrl);
    logTaskHandle = osThreadNew(Log_Task, NULL, &attr);
}

int Log_Printf(log_level_t level, const char *format, ...)
{
    log_message_t msg;
    va_list args;

    if (format == NULL) {
        return 0;
    }
    memset(&msg, 0, sizeof(msg));
    msg.level = level;
    va_start(args, format);
    vsnprintf(msg.message, sizeof(msg.message), format, args);
    va_end(args);

    if (logQueue != NULL && xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        (void)xQueueSend(logQueue, &msg, 0);
    }
    return (int)strlen(msg.message);
}

int Log_Print(log_level_t level, const char *message)
{
    return Log_Printf(level, "%s", (message != NULL) ? message : "(null)");
}
