/* OLED：0.91" SSD1306 128x32，I2C1 PB6 SCL / PB7 SDA，板级 4.7k 上拉 */
#include "lcd_task.h"
#include "lcd_pages.h"
#include "lcd_priv.h"
#include "user_config.h"
#include "key_task.h"
#include "i2c.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

#define SSD1306_CMD_SET_CONTRAST           0x81
#define SSD1306_CMD_DISPLAY_ALL_ON_RESUME  0xA4
#define SSD1306_CMD_NORMAL_DISPLAY         0xA6
#define SSD1306_CMD_DISPLAY_OFF            0xAE
#define SSD1306_CMD_DISPLAY_ON             0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET     0xD3
#define SSD1306_CMD_SET_COMPINS            0xDA
#define SSD1306_CMD_SET_VCOM_DETECT        0xDB
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIV  0xD5
#define SSD1306_CMD_SET_PRECHARGE          0xD9
#define SSD1306_CMD_SET_MULTIPLEX          0xA8
#define SSD1306_CMD_SET_START_LINE         0x40
#define SSD1306_CMD_MEMORY_MODE            0x20
#define SSD1306_CMD_COLUMN_ADDR            0x21
#define SSD1306_CMD_PAGE_ADDR              0x22
#define SSD1306_CMD_COM_SCAN_INC           0xC0
#define SSD1306_CMD_COM_SCAN_DEC           0xC8
#define SSD1306_CMD_SEG_REMAP              0xA0
#define SSD1306_CMD_CHARGE_PUMP            0x8D

static osThreadId_t lcdTaskHandle;
static osMutexId_t lcdMutexHandle;
static StackType_t lcdTaskStack[USER_CONFIG_LCD_TASK_STACK_SIZE];
static StaticTask_t lcdTaskTCB;

uint8_t lcd_buffer[LCD_WIDTH * LCD_PAGES];
bool lcd_buffer_dirty;
lcd_status_t lcd_status;
static uint8_t s_i2c_addr = (uint8_t)(USER_CONFIG_LCD_I2C_ADDR_7BIT << 1);

/* control：0x00 命令，0x40 数据。每次最多带 16 字节，避免大栈缓冲 */
static HAL_StatusTypeDef LCD_I2C_Write(uint8_t control, const uint8_t *data, uint16_t len)
{
    uint8_t chunk[17];
    uint16_t offset = 0;

    if (len == 0u) {
        chunk[0] = control;
        return HAL_I2C_Master_Transmit(&hi2c1, s_i2c_addr, chunk, 1, USER_CONFIG_LCD_I2C_TIMEOUT_MS);
    }

    while (offset < len) {
        uint16_t n = (uint16_t)(len - offset);
        if (n > 16u) {
            n = 16u;
        }
        chunk[0] = control;
        memcpy(&chunk[1], &data[offset], n);
        if (HAL_I2C_Master_Transmit(&hi2c1, s_i2c_addr, chunk, (uint16_t)(n + 1u),
                                    USER_CONFIG_LCD_I2C_TIMEOUT_MS) != HAL_OK) {
            return HAL_ERROR;
        }
        offset = (uint16_t)(offset + n);
    }
    return HAL_OK;
}

static void LCD_WriteCommand(uint8_t cmd)
{
    (void)LCD_I2C_Write(0x00u, &cmd, 1u);   /* Co=0, D/C#=0：命令 */
}

static void LCD_WriteDataBlock(const uint8_t *data, uint16_t len)
{
    (void)LCD_I2C_Write(0x40u, data, len);  /* Co=0, D/C#=1：显存数据 */
}

/* 按 128x32 配置：MUX=31，COM pins=0x02 */
static void SSD1306_Init_Sequence(void)
{
    LCD_WriteCommand(SSD1306_CMD_DISPLAY_OFF);
    LCD_WriteCommand(SSD1306_CMD_SET_DISPLAY_CLOCK_DIV);
    LCD_WriteCommand(0x80);
    LCD_WriteCommand(SSD1306_CMD_SET_MULTIPLEX);
    LCD_WriteCommand((uint8_t)(LCD_HEIGHT - 1));
    LCD_WriteCommand(SSD1306_CMD_SET_DISPLAY_OFFSET);
    LCD_WriteCommand(0x00);
    LCD_WriteCommand(SSD1306_CMD_SET_START_LINE | 0x00);
    LCD_WriteCommand(SSD1306_CMD_CHARGE_PUMP);
    LCD_WriteCommand(0x14);
    LCD_WriteCommand(SSD1306_CMD_MEMORY_MODE);
    LCD_WriteCommand(0x00);
#if USER_CONFIG_LCD_ROTATE_180
    LCD_WriteCommand(SSD1306_CMD_SEG_REMAP | 0x0);
    LCD_WriteCommand(SSD1306_CMD_COM_SCAN_INC);
#else
    LCD_WriteCommand(SSD1306_CMD_SEG_REMAP | 0x1);
    LCD_WriteCommand(SSD1306_CMD_COM_SCAN_DEC);
#endif
    LCD_WriteCommand(SSD1306_CMD_SET_COMPINS);
    LCD_WriteCommand((LCD_HEIGHT == 32) ? 0x02u : 0x12u);
    LCD_WriteCommand(SSD1306_CMD_SET_CONTRAST);
    LCD_WriteCommand(0xCF);
    LCD_WriteCommand(SSD1306_CMD_SET_PRECHARGE);
    LCD_WriteCommand(0xF1);
    LCD_WriteCommand(SSD1306_CMD_SET_VCOM_DETECT);
    LCD_WriteCommand(0x40);
    LCD_WriteCommand(SSD1306_CMD_NORMAL_DISPLAY);
    LCD_WriteCommand(SSD1306_CMD_DISPLAY_ALL_ON_RESUME);
    LCD_WriteCommand(SSD1306_CMD_DISPLAY_ON);
}

static bool LCD_ProbeAddress(uint8_t addr7)
{
    uint16_t addr = (uint16_t)(addr7 << 1);
    return (HAL_I2C_IsDeviceReady(&hi2c1, addr, 3, 50) == HAL_OK);
}

bool LCD_Init(void)
{
    if (lcdMutexHandle == NULL) {
        return false;
    }
    if (osMutexAcquire(lcdMutexHandle, 1000) != osOK) {
        return false;
    }

    /* 先探 0x3C，没有再试 0x3D */
    if (LCD_ProbeAddress(USER_CONFIG_LCD_I2C_ADDR_7BIT)) {
        s_i2c_addr = (uint8_t)(USER_CONFIG_LCD_I2C_ADDR_7BIT << 1);
    } else if (LCD_ProbeAddress(0x3Du)) {
        s_i2c_addr = (uint8_t)(0x3Du << 1);
    } else {
        osMutexRelease(lcdMutexHandle);
        return false;
    }

    osDelay(10);
    SSD1306_Init_Sequence();
    memset(lcd_buffer, 0, sizeof(lcd_buffer));
    lcd_buffer_dirty = true;
    lcd_status.initialized = true;
    lcd_status.display_on = true;
    lcd_status.contrast = 0xCF;
    osMutexRelease(lcdMutexHandle);
    LCD_Refresh();
    return true;
}

void LCD_Clear(void)
{
    if (!lcd_status.initialized) {
        return;
    }
    if (osMutexAcquire(lcdMutexHandle, 100) == osOK) {
        memset(lcd_buffer, 0, sizeof(lcd_buffer));
        lcd_buffer_dirty = true;
        osMutexRelease(lcdMutexHandle);
    }
    LCD_Refresh();
}

void LCD_Refresh(void)
{
    if (!lcd_status.initialized || !lcd_status.display_on) {
        return;
    }
    if (osMutexAcquire(lcdMutexHandle, 100) != osOK) {
        return;
    }
    if (!lcd_buffer_dirty) {
        osMutexRelease(lcdMutexHandle);
        return;
    }
    LCD_WriteCommand(SSD1306_CMD_COLUMN_ADDR);
    LCD_WriteCommand(0);
    LCD_WriteCommand(LCD_WIDTH - 1);
    LCD_WriteCommand(SSD1306_CMD_PAGE_ADDR);
    LCD_WriteCommand(0);
    LCD_WriteCommand(LCD_PAGES - 1);
    LCD_WriteDataBlock(lcd_buffer, sizeof(lcd_buffer));
    lcd_buffer_dirty = false;
    osMutexRelease(lcdMutexHandle);
}

void LCD_SetPixel(uint8_t x, uint8_t y, bool color)
{
    uint8_t page;
    uint8_t bit;
    uint16_t index;

    if (!lcd_status.initialized || x >= LCD_WIDTH || y >= LCD_HEIGHT) {
        return;
    }
    if (osMutexAcquire(lcdMutexHandle, 10) != osOK) {
        return;
    }
    /* SSD1306 页模式：一页 8 行像素 */
    page = (uint8_t)(y / 8u);
    bit = (uint8_t)(y % 8u);
    index = (uint16_t)(page * LCD_WIDTH + x);
    if (color) {
        lcd_buffer[index] |= (uint8_t)(1u << bit);
    } else {
        lcd_buffer[index] &= (uint8_t)~(1u << bit);
    }
    lcd_buffer_dirty = true;
    osMutexRelease(lcdMutexHandle);
}

static void LCD_PrintChar_Internal(uint8_t x, uint8_t y, char ch, font_size_t font_size)
{
    uint8_t num_cols;
    uint8_t col;

    if (!lcd_status.initialized || y >= LCD_PAGES || x >= LCD_WIDTH) {
        return;
    }
    if (ch < 32 || ch > 126) {
        ch = 32;
    }

    if (font_size == FONT_SIZE_6X8) {
        const uint8_t *font_data = font_6x8[ch - 32];
        num_cols = 6;
        if ((x + num_cols) > LCD_WIDTH) {
            num_cols = (uint8_t)(LCD_WIDTH - x);
        }
        for (col = 0; col < num_cols; col++) {
            lcd_buffer[y * LCD_WIDTH + x + col] = font_data[col];
        }
    } else if (font_size == FONT_SIZE_8X8) {
        const uint8_t *font_data = font_8x8[ch - 32];
        num_cols = 8;
        if ((x + num_cols) > LCD_WIDTH) {
            num_cols = (uint8_t)(LCD_WIDTH - x);
        }
        for (col = 0; col < num_cols; col++) {
            lcd_buffer[y * LCD_WIDTH + x + col] = font_data[col];
        }
    } else {
        /* 8x16 未编入，退回 8x8 以节省 Flash */
        const uint8_t *font_data = font_8x8[ch - 32];
        num_cols = 8;
        if ((x + num_cols) > LCD_WIDTH) {
            num_cols = (uint8_t)(LCD_WIDTH - x);
        }
        for (col = 0; col < num_cols; col++) {
            lcd_buffer[y * LCD_WIDTH + x + col] = font_data[col];
        }
    }
}

void LCD_PrintString_Internal(uint8_t x, uint8_t y, const char *str, font_size_t font_size)
{
    uint8_t char_width = (font_size == FONT_SIZE_6X8) ? 6u : 8u;
    uint8_t pos_x = x;

    if (!lcd_status.initialized || str == NULL) {
        return;
    }
    while (*str && pos_x < LCD_WIDTH) {
        LCD_PrintChar_Internal(pos_x, y, *str, font_size);
        pos_x = (uint8_t)(pos_x + char_width);
        str++;
    }
}

void LCD_PrintChar(uint8_t x, uint8_t y, char ch, font_size_t font_size)
{
    if (osMutexAcquire(lcdMutexHandle, 10) != osOK) {
        return;
    }
    LCD_PrintChar_Internal(x, y, ch, font_size);
    lcd_buffer_dirty = true;
    osMutexRelease(lcdMutexHandle);
}

void LCD_PrintString(uint8_t x, uint8_t y, const char *str, font_size_t font_size)
{
    if (osMutexAcquire(lcdMutexHandle, 10) != osOK) {
        return;
    }
    LCD_PrintString_Internal(x, y, str, font_size);
    lcd_buffer_dirty = true;
    osMutexRelease(lcdMutexHandle);
}

void LCD_PrintNumber(uint8_t x, uint8_t y, int32_t num, font_size_t font_size)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%ld", (long)num);
    LCD_PrintString(x, y, buffer, font_size);
}

void LCD_PrintFloat(uint8_t x, uint8_t y, float num, uint8_t decimals, font_size_t font_size)
{
    char buffer[32];
    char format[8];
    snprintf(format, sizeof(format), "%%.%uf", (unsigned)decimals);
    snprintf(buffer, sizeof(buffer), format, (double)num);
    LCD_PrintString(x, y, buffer, font_size);
}

void LCD_Printf(font_size_t font_size, uint8_t row, uint8_t col, const char *format, ...)
{
    uint8_t char_width = (font_size == FONT_SIZE_6X8) ? 6u : 8u;
    uint8_t char_height_pages = (font_size == FONT_SIZE_8X16) ? 2u : 1u;
    char buffer[64];
    va_list args;

    if (!lcd_status.initialized || format == NULL) {
        return;
    }
    if (osMutexAcquire(lcdMutexHandle, 10) != osOK) {
        return;
    }
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    LCD_PrintString_Internal((uint8_t)(col * char_width),
                             (uint8_t)(row * char_height_pages),
                             buffer, font_size);
    lcd_buffer_dirty = true;
    osMutexRelease(lcdMutexHandle);
}

void LCD_DrawHLine(uint8_t x, uint8_t y, uint8_t width)
{
    uint8_t i;
    for (i = 0; i < width && (x + i) < LCD_WIDTH; i++) {
        LCD_SetPixel((uint8_t)(x + i), y, true);
    }
}

void LCD_DrawVLine(uint8_t x, uint8_t y, uint8_t height)
{
    uint8_t i;
    for (i = 0; i < height && (y + i) < LCD_HEIGHT; i++) {
        LCD_SetPixel(x, (uint8_t)(y + i), true);
    }
}

void LCD_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled)
{
    uint8_t i;
    if (filled) {
        for (i = 0; i < height && (y + i) < LCD_HEIGHT; i++) {
            LCD_DrawHLine(x, (uint8_t)(y + i), width);
        }
    } else {
        LCD_DrawHLine(x, y, width);
        LCD_DrawHLine(x, (uint8_t)(y + height - 1u), width);
        LCD_DrawVLine(x, y, height);
        LCD_DrawVLine((uint8_t)(x + width - 1u), y, height);
    }
}

void LCD_SetDisplayOn(bool on)
{
    if (!lcd_status.initialized) {
        return;
    }
    LCD_WriteCommand(on ? SSD1306_CMD_DISPLAY_ON : SSD1306_CMD_DISPLAY_OFF);
    lcd_status.display_on = on;
}

void LCD_SetContrast(uint8_t contrast)
{
    if (!lcd_status.initialized) {
        return;
    }
    LCD_WriteCommand(SSD1306_CMD_SET_CONTRAST);
    LCD_WriteCommand(contrast);
    lcd_status.contrast = contrast;
}

bool LCD_GetStatus(lcd_status_t *status)
{
    if (status == NULL) {
        return false;
    }
    *status = lcd_status;
    return true;
}

static void lcd_on_key(const key_event_t *evt, void *user)
{
    (void)user;
    if (evt == NULL) {
        return;
    }
    lcd_page_home_handle_key(evt->key_id, evt->event_type);
}

static void LCD_Task(void *argument)
{
    (void)argument;

    if (!LCD_Init()) {
        /* 屏不在就停在这里，避免空刷 I2C */
        for (;;) {
            osDelay(1000);
        }
    }
    (void)Key_RegisterCallback(lcd_on_key, NULL);

    for (;;) {
        if (osMutexAcquire(lcdMutexHandle, 50) == osOK) {
            memset(lcd_buffer, 0, sizeof(lcd_buffer));
            lcd_page_home_render();
            lcd_buffer_dirty = true;
            osMutexRelease(lcdMutexHandle);
        }
        LCD_Refresh();
        osDelay(USER_CONFIG_LCD_REFRESH_PERIOD_MS);
    }
}

void LCD_Task_Create(void)
{
    static const osMutexAttr_t mutex_attr = { .name = "LCD_Mutex" };
    const osThreadAttr_t attr = {
        .name = "LCD_Task",
        .cb_mem = &lcdTaskTCB,
        .cb_size = sizeof(lcdTaskTCB),
        .stack_mem = lcdTaskStack,
        .stack_size = sizeof(lcdTaskStack),
        .priority = (osPriority_t)USER_CONFIG_LCD_TASK_PRIORITY,
    };

    if (lcdTaskHandle != NULL) {
        return;
    }
    lcdMutexHandle = osMutexNew(&mutex_attr);
    if (lcdMutexHandle == NULL) {
        return;
    }
    lcdTaskHandle = osThreadNew(LCD_Task, NULL, &attr);
}
