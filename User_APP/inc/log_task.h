#ifndef LOG_TASK_H
#define LOG_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_MAX
} log_level_t;

void Log_Task_Create(void);
int Log_Printf(log_level_t level, const char *format, ...);
int Log_Print(log_level_t level, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* LOG_TASK_H */
