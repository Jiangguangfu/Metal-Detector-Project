#ifndef LCD_PAGES_H
#define LCD_PAGES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 主页绘制（调用方已持有 LCD 互斥量） */
void lcd_page_home_render(void);
void lcd_page_home_handle_key(uint8_t key_id, uint8_t event_type);

#ifdef __cplusplus
}
#endif

#endif /* LCD_PAGES_H */
