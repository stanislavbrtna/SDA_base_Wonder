/*
Copyright (c) 2018 Stanislav Brtna

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H
#include "../sda_platform.h"

/* for the vision HW

#define LCD_DAT_PORT GPIOC

#define LCD_RST_PORT GPIOC
#define LCD_RST_PIN  GPIO_PIN_9

#define LCD_CS_PORT  GPIOC
#define LCD_CS_PIN   GPIO_PIN_8

#define LCD_RS_PORT  GPIOA
#define LCD_RS_PIN   GPIO_PIN_8

#define LCD_WR_PORT  GPIOA
#define LCD_WR_PIN   GPIO_PIN_11

#define LCD_RD_PORT  GPIOA
#define LCD_RD_PIN   GPIO_PIN_12

#define LCD_BL_PORT GPIOA
#define LCD_BL_PIN  GPIO_PIN_1
#define LCD_BL_ALT  GPIO_AF1_TIM2

#define LCD_BL_TIMER   TIM2
#define LCD_BL_CHANNEL TIM_CHANNEL_2
*/

#define LCD_DAT_PORT GPIOC

#define LCD_RST_PORT GPIOA
#define LCD_RST_PIN  GPIO_PIN_9

#define LCD_CS_PORT  GPIOA
#define LCD_CS_PIN   GPIO_PIN_10

#define LCD_RS_PORT  GPIOA
#define LCD_RS_PIN   GPIO_PIN_8

#define LCD_WR_PORT  GPIOA
#define LCD_WR_PIN   GPIO_PIN_11

#define LCD_RD_PORT  GPIOA
#define LCD_RD_PIN   GPIO_PIN_12

#define LCD_BL_PORT GPIOE
#define LCD_BL_PIN  GPIO_PIN_6
#define LCD_BL_ALT  GPIO_AF3_TIM9

#define LCD_BL_TIMER   TIM9
#define LCD_BL_CHANNEL TIM_CHANNEL_2

#define LCD_TIM_CLK_ENABLE __TIM9_CLK_ENABLE()

// GR2 hooks
uint8_t lcd_hw_init();
void lcd_set_params(uint8_t gamma_mode, uint8_t invert);
extern void lcd_hw_set_xy(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
extern void lcd_hw_Draw_Point(uint16_t color);

// SDA_OS hooks:
void lcd_hw_sleep();
void lcd_hw_wake();
void lcd_bl_on();
void lcd_bl_off();
void lcd_bl_timer_OC_update();

void backlight_timer_init();
void lcd_hw_set_backlight(uint8_t val0To255);

//

#endif
