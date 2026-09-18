#ifndef USER_MAIN_H
#define USER_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* USB 初始化完成后由 defaultTask 调用，创建全部 User_APP 任务 */
void user_main(void);

#ifdef __cplusplus
}
#endif

#endif /* USER_MAIN_H */
