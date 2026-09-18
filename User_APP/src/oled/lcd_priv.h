#ifndef LCD_PRIV_H
#define LCD_PRIV_H

#include "lcd_task.h"
#include "font_8x16.h"
#include <stdint.h>
#include <stdbool.h>

extern uint8_t lcd_buffer[];
extern bool lcd_buffer_dirty;
extern lcd_status_t lcd_status;

void LCD_PrintString_Internal(uint8_t x, uint8_t y, const char *str, font_size_t font_size);

#endif /* LCD_PRIV_H */
