/* -*- coding: utf-8 -*- */
/**
  ******************************************************************************
  * @file    font_8x16.h
  * @brief   8x16、8x8和6x8字体数据定义
  ******************************************************************************
  */

#ifndef FONT_8X16_H
#define FONT_8X16_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 字体大小枚举 */
typedef enum {
    FONT_SIZE_6X8 = 0,   /* 6x8字体 */
    FONT_SIZE_8X8 = 1,   /* 8x8字体 */
    FONT_SIZE_8X16 = 2   /* 8x16字体 */
} font_size_t;

#if 0
/* 8x16字体数据（ASCII 32-126）— F103C8 未链接此表 */
extern const uint8_t font_8x16[][16];
#endif

/* 8x8字体数据（ASCII 32-126） */
extern const uint8_t font_8x8[][8];

/* 6x8字体数据（ASCII 32-126） */
extern const uint8_t font_6x8[][6];

#ifdef __cplusplus
}
#endif

#endif /* FONT_8X16_H */

