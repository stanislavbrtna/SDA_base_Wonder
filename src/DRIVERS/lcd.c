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

#include <string.h>
#include "lcd.h"

//optimised GPIO manipulation macros
#define lcd_set_RST_low() do { LCD_RST_PORT->BSRR = (uint32_t)LCD_RST_PIN << 16; } while (0)
#define lcd_set_RST_high() do { LCD_RST_PORT->BSRR = LCD_RST_PIN; } while (0)

#define lcd_set_CS_low() do { LCD_CS_PORT->BSRR = (uint32_t)LCD_CS_PIN << 16; } while (0)
#define lcd_set_CS_high() do { LCD_CS_PORT->BSRR = LCD_CS_PIN; } while (0)

#define lcd_set_RS_low() do { LCD_RS_PORT->BSRR = (uint32_t)LCD_RS_PIN << 16; } while (0)
#define lcd_set_RS_high() do { LCD_RS_PORT->BSRR = LCD_RS_PIN; } while (0)

#define lcd_set_WR_low() do { LCD_WR_PORT->BSRR = (uint32_t)LCD_WR_PIN << 16; } while (0)
#define lcd_set_WR_high() do { LCD_WR_PORT->BSRR = LCD_WR_PIN; } while (0)

#define lcd_set_RD_low() do { LCD_RD_PORT->BSRR = (uint32_t)LCD_RD_PIN << 16; } while (0)
#define lcd_set_RD_high() do { LCD_RD_PORT->BSRR = LCD_RD_PIN; } while (0)

// internal functions headers
inline void lcd_Delay(__IO uint32_t nCount);
static void LCD_Write_COM(uint8_t x);
static void LCD_Write_DATA(uint8_t x);


void lcd_GPIO_Init();
void lcd_GPIO_Init_input();
void lcd_GPIO_Init_output();

void writeRegister16(uint8_t x, uint16_t data);

// send data/cmd functions
inline void lcd_send_data(uint8_t data)  __attribute__((always_inline));
inline void lcd_send_cmd(uint8_t data) __attribute__((always_inline));

// same functions with delay
void lcd_send_cmd_d(uint8_t data); 
static void lcd_send_data_d(uint8_t data);

/*
 * for GR2:
 *
 * uint8_t lcd_hw_init();
 * void lcd_hw_Draw_Point(uint16_t color);
 * void lcd_hw_set_xy(uint16_t px1, uint16_t py1, uint16_t px2, uint16_t py2);
 *
 * for SDA_OS:
 *
 * void lcd_hw_sleep();
 * void lcd_hw_wake();
 *
 */


uint8_t lcd_detect_type();

void lcd_Init_Seq_9488();
void lcd_Init_Seq_9481_b();
void lcd_Init_Seq_9488_a();

void lcd_Init_Seq_9481();

void set_9488_gamma(uint8_t val);

#define LCD_TYPE_9481 1
#define LCD_TYPE_9488 2

uint8_t LCD_type;
uint8_t LCD_delay;
uint8_t LCD_gamma_mode;
uint8_t LCD_invert;

void lcd_set_params(uint8_t gamma_mode, uint8_t invert) {
  LCD_gamma_mode = gamma_mode;
  LCD_invert = invert;
}

uint8_t lcd_hw_init() {
  lcd_GPIO_Init();
  lcd_set_RD_high();
  lcd_set_RS_high();
  LCD_delay = 50;

  LCD_type = lcd_detect_type();

  printf("info: LCD_type: %u\n", LCD_type);

  if (LCD_type == LCD_TYPE_9488) {
    // TODO: clean up the 9488 init seq.
    LCD_delay = 50;
    lcd_Init_Seq_9488_a(); //standard
    LCD_delay = 0;
  } else {
    LCD_delay = 50;
    lcd_Init_Seq_9481_b();
    LCD_delay = 1;
  }

  return 0;
}

uint8_t lcd_detect_type(){
  // read ID
  uint8_t d[8];
  memset(d, 0, sizeof d);

  lcd_set_RST_low();
  lcd_Delay(300);
  lcd_set_RST_high();
  lcd_Delay(300);

  lcd_send_cmd_d(0xBF); //ID4
  lcd_GPIO_Init_input();
  lcd_Delay(100);
  lcd_set_RD_low();
  lcd_Delay(100);
  lcd_set_RD_high();
  lcd_Delay(20);

  for(uint16_t i=0; i<4; i++) {
    lcd_set_RD_low();
    lcd_Delay(100);
    d[i] = LCD_DAT_PORT->IDR;
    printf("LCD Data%u: %x\n", i, d[i]);
    lcd_set_RD_high();
    lcd_Delay(100);
  }
  lcd_GPIO_Init_output();

  if(d[2] == 0x94 && d[3] == 0x81) {
    return LCD_TYPE_9481;
  }

  lcd_send_cmd_d(0xD3); //ID4
  lcd_GPIO_Init_input();
  lcd_Delay(100);
  lcd_set_RD_low();
  lcd_Delay(100);
  lcd_set_RD_high();
  lcd_Delay(20);

  for(uint16_t i=0; i<3; i++) {
    lcd_set_RD_low();
    lcd_Delay(10);
    d[i] = LCD_DAT_PORT->IDR;
    printf("LCD Data%u: %x\n", i, d[i]);
    lcd_set_RD_high();
    lcd_Delay(10);
  }
  lcd_GPIO_Init_output();

  if(d[1] == 0x94 && d[2] == 0x88) {
    return LCD_TYPE_9488;
  }

  return 0;
}

void lcd_hw_sleep() {
  LCD_Write_COM(0x10);
  lcd_Delay(20);
}

void lcd_hw_wake() {
  LCD_Write_COM(0x11);
  lcd_Delay(20);
}


inline void lcd_Delay(__IO uint32_t nCount) {
  nCount *= 3;
  for(; nCount != 0; nCount--);
}

static void LCD_Write_COM(uint8_t x) {
  lcd_send_cmd(x);
}

static void LCD_Write_DATA(uint8_t x) {
  lcd_send_data(x);
}

void writeRegister16(uint8_t x,uint16_t data) {
  lcd_send_cmd(x);
  lcd_send_data(data >> 8);
  lcd_send_data(0x00FF & data);
}

void lcd_Init_Seq_9488_a() {

  lcd_set_RST_low();
  lcd_Delay(300);
  lcd_set_RST_high();
  lcd_Delay(300);

  lcd_set_RD_high();
  lcd_set_CS_low();

  lcd_Delay(50);
  // The send data D function stands for a bit of delay, LCD likes it better upon init
  lcd_send_cmd_d(0xF1);  //exit sleep

  lcd_send_cmd_d(0x3A);  // interface pixel format
  lcd_send_data(0x55);   // 16 bit pixel

  //lcd_send_cmd_d(0XC0);   //Power Control 1
  //lcd_send_data(0x1F);    //Verg1out 17
  //lcd_send_data(0x01);    //Vreg2out 15

  //lcd_send_cmd_d(0xC1);   //Power Control 2
  //lcd_send_data(0x44);    //VGH,VGL //41 og

  //lcd_send_cmd_d(0xC2);  // Power Control 3 (for normal mode)
  //lcd_send_data(0x22);   // og 33h

  //lcd_send_cmd_d(0xC5);   //Power Control 3
  //lcd_send_data(0x00);
  //lcd_send_data(0x12);    //Vcom
  //lcd_send_data(0x80);


  set_9488_gamma(LCD_gamma_mode);

  lcd_send_cmd_d(0x36);  // set address mode
    lcd_send_data(0x48);

  lcd_send_cmd_d(0x11);  // sleep out
  lcd_Delay(150);

  if(LCD_invert) {
    lcd_send_cmd_d(0x21); // enter_invert_mode
  }

  lcd_send_cmd_d(0x29);   // display on
  lcd_Delay(150);
}

void set_9488_gamma(uint8_t val) {
  printf("setting gamma: %u\n", val);
  if(val == 0) {
    lcd_send_cmd_d(0xE0);
      lcd_send_data(0x00);
      lcd_send_data(0x07);
      lcd_send_data(0x0C);
      lcd_send_data(0x05);
      lcd_send_data(0x13);
      lcd_send_data(0x09);
      lcd_send_data(0x36);
      lcd_send_data(0xAA);
      lcd_send_data(0x46);
      lcd_send_data(0x09);
      lcd_send_data(0x10);
      lcd_send_data(0x0D);
      lcd_send_data(0x1A);
      lcd_send_data(0x1E);
      lcd_send_data(0x0F);

    lcd_send_cmd_d(0xE1);
      lcd_send_data(0x00);
      lcd_send_data(0x20);
      lcd_send_data(0x23);
      lcd_send_data(0x04);
      lcd_send_data(0x10);
      lcd_send_data(0x06);
      lcd_send_data(0x37);
      lcd_send_data(0x56);
      lcd_send_data(0x49);
      lcd_send_data(0x04);
      lcd_send_data(0x0C);
      lcd_send_data(0x0A);
      lcd_send_data(0x33);
      lcd_send_data(0x37);
      lcd_send_data(0x0F);
  } else if(val == 1) {
    lcd_send_cmd_d(0xE0);
      lcd_send_data(0x00);
      lcd_send_data(0x07);
      lcd_send_data(0x0f);
      lcd_send_data(0x0D);
      lcd_send_data(0x1B);
      lcd_send_data(0x0A);
      lcd_send_data(0x3c);
      lcd_send_data(0x78);
      lcd_send_data(0x4A);
      lcd_send_data(0x07);
      lcd_send_data(0x0E);
      lcd_send_data(0x09);
      lcd_send_data(0x1B);
      lcd_send_data(0x1e);
      lcd_send_data(0x0f);

    lcd_send_cmd_d(0xE1);
      lcd_send_data(0x00);
      lcd_send_data(0x22);
      lcd_send_data(0x24);
      lcd_send_data(0x06);
      lcd_send_data(0x12);
      lcd_send_data(0x07);
      lcd_send_data(0x36);
      lcd_send_data(0x47);
      lcd_send_data(0x47);
      lcd_send_data(0x06);
      lcd_send_data(0x0a);
      lcd_send_data(0x07);
      lcd_send_data(0x30);
      lcd_send_data(0x37);
      lcd_send_data(0x0f);
  } else if(val == 2) {
    lcd_send_cmd_d(0xE0); // gamma settings too dark, use those from 9486
      lcd_send_data_d(0x00);
      lcd_send_data_d(0x03);
      lcd_send_data_d(0x09);
      lcd_send_data_d(0x08);
      lcd_send_data_d(0x16);
      lcd_send_data_d(0x0A);
      lcd_send_data_d(0x3F);
      lcd_send_data_d(0x78);
      lcd_send_data_d(0x4C);
      lcd_send_data_d(0x09);
      lcd_send_data_d(0x0A);
      lcd_send_data_d(0x08);
      lcd_send_data_d(0x16);
      lcd_send_data_d(0x1A);
      lcd_send_data_d(0x0F);

    lcd_send_cmd_d(0XE1);
      lcd_send_data_d(0x00);
      lcd_send_data_d(0x16);
      lcd_send_data_d(0x19);
      lcd_send_data_d(0x03);
      lcd_send_data_d(0x0F);
      lcd_send_data_d(0x05);
      lcd_send_data_d(0x32);
      lcd_send_data_d(0x45);
      lcd_send_data_d(0x46);
      lcd_send_data_d(0x04);
      lcd_send_data_d(0x0E);
      lcd_send_data_d(0x0D);
      lcd_send_data_d(0x35);
      lcd_send_data_d(0x37);
      lcd_send_data_d(0x0F);
  } else if(val == 3) {
    // same as 0, just inverted
    lcd_send_cmd_d(0xE0);
      lcd_send_data(0x00);
      lcd_send_data(0x20);
      lcd_send_data(0x23);
      lcd_send_data(0x04);
      lcd_send_data(0x10);
      lcd_send_data(0x06);
      lcd_send_data(0x37);
      lcd_send_data(0x56);
      lcd_send_data(0x49);
      lcd_send_data(0x04);
      lcd_send_data(0x0C);
      lcd_send_data(0x0A);
      lcd_send_data(0x33);
      lcd_send_data(0x37);
      lcd_send_data(0x0F);

    lcd_send_cmd_d(0xE1);
      lcd_send_data(0x00);
      lcd_send_data(0x07);
      lcd_send_data(0x0C);
      lcd_send_data(0x05);
      lcd_send_data(0x13);
      lcd_send_data(0x09);
      lcd_send_data(0x36);
      lcd_send_data(0xAA);
      lcd_send_data(0x46);
      lcd_send_data(0x09);
      lcd_send_data(0x10);
      lcd_send_data(0x0D);
      lcd_send_data(0x1A);
      lcd_send_data(0x1E);
      lcd_send_data(0x0F);
  }

  // else: set nothing
}

void lcd_Init_Seq_9481_b() {

  lcd_set_RST_low();
  lcd_Delay(300);
  lcd_set_RST_high();
  lcd_Delay(300);

  lcd_set_RD_high();
  lcd_set_CS_low();

  lcd_send_cmd_d(0x11); // sleep out
  lcd_Delay(80);

  //TODO: Tweak power settings fot this screen type

  lcd_send_cmd_d(0xD0); // power settings
  lcd_send_data_d(0x07);
  lcd_send_data_d(0x41);
  lcd_send_data_d(0x1D);

  lcd_send_cmd_d(0xD1); // vcom
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x14);
  lcd_send_data_d(0x1b);


  lcd_send_cmd_d(0xD2);  // power setting normal mode
  lcd_send_data_d(0x01); // AP0 0 - off 1 - max ... 7 50%
  lcd_send_data_d(0x00); // 11 12

/* Default settings, no need to set them up.

  lcd_send_cmd_d(0xC0);  // panel driving settings
  lcd_send_data_d(0x10);
  lcd_send_data_d(0x3B);
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x02);
  lcd_send_data_d(0x11);
  lcd_send_data_d(0x00);

  lcd_send_cmd_d(0xC5);  // frame rate
  lcd_send_data_d(0x03); //72hz
*/

  // Gamma
  if(LCD_gamma_mode == 0) {
    lcd_send_cmd_d(0xC8); 
    lcd_send_data_d(0x00);
    lcd_send_data_d(0x32);
    lcd_send_data_d(0x36);
    lcd_send_data_d(0x45);
    lcd_send_data_d(0x06);
    lcd_send_data_d(0x16);
    lcd_send_data_d(0x37);
    lcd_send_data_d(0x75);
    lcd_send_data_d(0x77);
    lcd_send_data_d(0x54);
    lcd_send_data_d(0x0C);
    lcd_send_data_d(0x00);
  } else if (LCD_gamma_mode == 1) {
    lcd_send_cmd_d(0xC8);
    lcd_send_data_d(0x00);
    lcd_send_data_d(0x14);
    lcd_send_data_d(0x33);
    lcd_send_data_d(0x10);
    lcd_send_data_d(0x00);
    lcd_send_data_d(0x16);
    lcd_send_data_d(0x44);
    lcd_send_data_d(0x36);
    lcd_send_data_d(0x77);
    lcd_send_data_d(0x00);
    lcd_send_data_d(0x0F);
    lcd_send_data_d(0x00);
  } else {
    lcd_send_cmd_d(0xC8);
    lcd_send_data_d(0x37);
    lcd_send_data_d(0x75);
    lcd_send_data_d(0x77);
    lcd_send_data_d(0x54);
    lcd_send_data_d(0x0C);
    lcd_send_data_d(0x00);

    lcd_send_data_d(0x00);
    lcd_send_data_d(0x32);
    lcd_send_data_d(0x36);
    lcd_send_data_d(0x45);
    lcd_send_data_d(0x06);
    lcd_send_data_d(0x16);
  }

  lcd_send_cmd_d(0x36); // set address mode
  lcd_send_data_d(0x0A);

  lcd_send_cmd_d(0x3A); // set pixel format
  lcd_send_data_d(0x55);

  lcd_send_cmd_d(0x2A);  // set_column_address
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x01);
  lcd_send_data_d(0x3F);

  lcd_send_cmd_d(0x2B); // set_page_address
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x00);
  lcd_send_data_d(0x01);
  lcd_send_data_d(0xE0);

  if(LCD_invert) {
    lcd_send_cmd_d(0x21); // enter_invert_mode
  }

  lcd_Delay(120);
  lcd_send_cmd_d(0x29); // display on
}


inline void lcd_send_data(uint8_t data) {

  LCD_DAT_PORT->ODR &= 0xFF00;
  LCD_DAT_PORT->ODR |= data;

  lcd_set_WR_low();
  if(LCD_delay) {
    lcd_Delay(2); //5
  }
  lcd_set_WR_high();
}

inline void lcd_send_data_d(uint8_t data) {
  LCD_DAT_PORT->ODR &= 0xFF00;
  LCD_DAT_PORT->ODR |= data;

  lcd_set_WR_low();
  lcd_Delay(30);
  lcd_set_WR_high();
}

inline void lcd_send_cmd(uint8_t data) {
  lcd_set_RS_low();

  LCD_DAT_PORT->ODR &= 0xFF00;
  LCD_DAT_PORT->ODR |= data;

  lcd_set_WR_low();

  if(LCD_delay) {
    lcd_Delay(30); //30
  }

  lcd_set_WR_high();
  lcd_set_RS_high();
}

void lcd_send_cmd_d(uint8_t data) {
  lcd_set_RS_low();

  LCD_DAT_PORT->ODR &= 0xFF00;
  LCD_DAT_PORT->ODR |= data;

  lcd_set_WR_low();
  lcd_Delay(120);
  lcd_set_WR_high();
  lcd_set_RS_high();
}


inline __attribute__((always_inline)) void lcd_hw_set_xy(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  lcd_send_cmd(0x2A);
  lcd_send_data(x1 >> 8);
  lcd_send_data(0x00FF & x1);
  lcd_send_data(x2 >> 8);
  lcd_send_data(0x00FF & x2);

  lcd_send_cmd(0x2B);
  lcd_send_data(y1 >> 8);
  lcd_send_data(0x00FF & y1);
  lcd_send_data(y2 >> 8);
  lcd_send_data(0x00FF & y2);

  lcd_send_cmd(0x2C);
}

inline void lcd_hw_set_cursor(uint16_t Xpos, uint16_t Ypos) {
  lcd_send_cmd(0x2A);
  lcd_send_data(Xpos >> 8);
  lcd_send_data(0x00FF & Xpos);
  lcd_send_cmd(0x2B);
  lcd_send_data(Ypos >> 8);
  lcd_send_data(0x00FF & Ypos);
  lcd_send_cmd(0x2C);
}

inline __attribute__((always_inline)) void lcd_hw_Draw_Point(uint16_t color) {
  lcd_send_data(color >> 8);
  lcd_send_data(color & 0x00FF);
}

static void lcd_init_pin(GPIO_TypeDef *port, uint32_t pin) {
  GPIO_InitTypeDef GPIO_InitStructure;
  GPIO_InitStructure.Pin   = pin;
  GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStructure.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(port, &GPIO_InitStructure);
}

void lcd_GPIO_Init_input() {
  GPIO_InitTypeDef GPIO_InitStructure;
  //Data pins are Px0 - Px7
  GPIO_InitStructure.Pin = GPIO_PIN_0
                          |GPIO_PIN_1
                          |GPIO_PIN_2
                          |GPIO_PIN_3
                          |GPIO_PIN_4
                          |GPIO_PIN_5
                          |GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStructure.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStructure.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_DAT_PORT, &GPIO_InitStructure);
}

void lcd_GPIO_Init_output() {
  GPIO_InitTypeDef GPIO_InitStructure;
  //Data pins are Px0 - Px7
  GPIO_InitStructure.Pin = GPIO_PIN_0
                          |GPIO_PIN_1
                          |GPIO_PIN_2
                          |GPIO_PIN_3
                          |GPIO_PIN_4
                          |GPIO_PIN_5
                          |GPIO_PIN_6
                          |GPIO_PIN_7;
  GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
  GPIO_InitStructure.Pull  = GPIO_NOPULL;
  HAL_GPIO_Init(LCD_DAT_PORT, &GPIO_InitStructure);
}

void lcd_GPIO_Init() {
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();

  lcd_GPIO_Init_output();

  lcd_init_pin(LCD_RST_PORT, LCD_RST_PIN);
  lcd_init_pin(LCD_CS_PORT, LCD_CS_PIN);
  lcd_init_pin(LCD_RS_PORT, LCD_RS_PIN);
  lcd_init_pin(LCD_WR_PORT, LCD_WR_PIN);
  lcd_init_pin(LCD_RD_PORT, LCD_RD_PIN);
}
