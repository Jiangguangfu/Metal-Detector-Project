#ifndef LCD_TASK_H
#define LCD_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "font_8x16.h"

#define LCD_WIDTH   128
#define LCD_HEIGHT  32          /* 0.91" SSD1306，不是 128x64 */
#define LCD_PAGES   (LCD_HEIGHT / 8)

typedef struct {
    bool initialized;
    bool display_on;
    uint8_t contrast;
} lcd_status_t;

void LCD_Task_Create(void);
bool LCD_Init(void);
void LCD_Clear(void);
void LCD_Refresh(void);         /* 把脏页刷到屏上 */
void LCD_SetPixel(uint8_t x, uint8_t y, bool color);
void LCD_PrintChar(uint8_t x, uint8_t y, char ch, font_size_t font_size);
void LCD_PrintString(uint8_t x, uint8_t y, const char *str, font_size_t font_size);
void LCD_PrintNumber(uint8_t x, uint8_t y, int32_t num, font_size_t font_size);
void LCD_PrintFloat(uint8_t x, uint8_t y, float num, uint8_t decimals, font_size_t font_size);
void LCD_Printf(font_size_t font_size, uint8_t row, uint8_t col, const char *format, ...);
void LCD_DrawHLine(uint8_t x, uint8_t y, uint8_t width);
void LCD_DrawVLine(uint8_t x, uint8_t y, uint8_t height);
void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled);
void LCD_SetDisplayOn(bool on);
void LCD_SetContrast(uint8_t contrast);
bool LCD_GetStatus(lcd_status_t *status);

#ifdef __cplusplus
}
#endif

#endif /* LCD_TASK_H */
