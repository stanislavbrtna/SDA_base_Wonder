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

TIM_HandleTypeDef blTimer;

// internal functions headers
static void lcd_Delay(__IO uint32_t nCount);
static void LCD_Write_COM(uint8_t x);
static void LCD_Write_DATA(uint8_t x);
static void setBacklight(TIM_HandleTypeDef timer, uint32_t channel, uint16_t pulse);

void lcd_GPIO_Init();

void writeRegister16(uint8_t x, uint16_t data);

//actual send function
static void lcd_send_data(uint8_t data);
static void lcd_send_cmd(uint8_t data);

void lcd_send_cmd_d(uint8_t data); // send with delay
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
 * void lcd_bl_on();
 * void lcd_bl_off();
 *
 * void lcd_hw_set_backlight(uint8_t val);
 *
 *
 */


//TODO: Add lcd panel type detection
//#define LCD_TYPE_B

void lcd_Init_Seq_9486();
void lcd_Init_Seq_9481_b();

void lcd_Init_Seq_9481();

uint8_t lcd_hw_init(){
	lcd_GPIO_Init();
#ifdef LCD_TYPE_B
	lcd_Init_Seq_9481_b();
#else
	lcd_Init_Seq_9486();
#endif
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

void lcd_bl_on() {
	GPIO_InitTypeDef GPIO_InitStructure;
	HAL_GPIO_DeInit(LCD_BL_PORT, LCD_BL_PIN);
	GPIO_InitStructure.Pin       = LCD_BL_PIN;
	GPIO_InitStructure.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStructure.Speed     = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull      = GPIO_PULLUP;
	GPIO_InitStructure.Alternate = LCD_BL_ALT;
	HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStructure);
}

void lcd_bl_off() {
	GPIO_InitTypeDef GPIO_InitStructure;
	//svp_set_backlight(0); // just why?
	HAL_TIM_PWM_Stop(&blTimer, LCD_BL_CHANNEL);

	HAL_GPIO_DeInit(LCD_BL_PORT, LCD_BL_PIN);
	GPIO_InitStructure.Pin   = LCD_BL_PIN;
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;
	HAL_GPIO_Init(LCD_BL_PORT, &GPIO_InitStructure);

	HAL_GPIO_WritePin(LCD_BL_PORT, LCD_BL_PIN, 0);
}

void lcd_hw_set_backlight(uint8_t val) {
	setBacklight(blTimer, LCD_BL_CHANNEL, val);
}

void backlight_timer_init() {
	LCD_TIM_CLK_ENABLE;
	lcd_bl_on();
	TIM_OC_InitTypeDef sConfigOC;

	blTimer.Instance         = LCD_BL_TIMER;
	blTimer.Channel          = HAL_TIM_ACTIVE_CHANNEL_2;
	blTimer.Init.Prescaler   = SystemCoreClock / 512;
	blTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
	blTimer.Init.Period      = 256;
	blTimer.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
	blTimer.Init.RepetitionCounter = 0;

	if(HAL_TIM_PWM_Init(&blTimer) != HAL_OK) {
		printf("HAL: TIM2 init error!\n");
	}
	/*
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;

	if(HAL_TIMEx_MasterConfigSynchronization(&blTimer, &sMasterConfig) != HAL_OK) {
		printf("HAL: TIM2 init error (2)!\n");
	}
	*/

	sConfigOC.OCMode       = TIM_OCMODE_PWM1;
	sConfigOC.Pulse        = 128;
	sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
	sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	if(HAL_TIM_PWM_ConfigChannel(&blTimer, &sConfigOC, LCD_BL_CHANNEL) != HAL_OK) {
		printf("HAL: TIM2 init error (3)!\n");
	}

	if(HAL_TIM_PWM_Start(&blTimer, LCD_BL_CHANNEL) != HAL_OK) {
		printf("HAL: TIM2 init error!(4)\n");
	}
}

void lcd_bl_timer_OC_update() {
	HAL_TIM_PWM_Stop(&blTimer, LCD_BL_CHANNEL);
	HAL_TIM_PWM_DeInit(&blTimer);
	// stop generation of pwm
	blTimer.Init.Prescaler = SystemCoreClock / 200000;
	if (HAL_TIM_PWM_Init(&blTimer)!= HAL_OK) {
		printf("HAL: TIM2 setPWM error (0)!\n");
	}
	if (HAL_TIM_PWM_Start(&blTimer, LCD_BL_CHANNEL)!= HAL_OK) {
		printf("HAL: TIM2 setPWM error (2)!\n");
	}
}

static void setBacklight(TIM_HandleTypeDef timer, uint32_t channel, uint16_t pulse) {
	TIM_OC_InitTypeDef sConfigOC;

	sConfigOC.OCMode       = TIM_OCMODE_PWM1;
	sConfigOC.Pulse        = pulse;
	sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCNPolarity  = TIM_OCNPOLARITY_LOW;
	sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
	sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
	sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;

	if (HAL_TIM_PWM_ConfigChannel(&timer, &sConfigOC, channel) != HAL_OK) {
		printf("HAL: TIM2 setPWM error (1)!\n");
	}

	if (HAL_TIM_PWM_Start(&timer, channel) != HAL_OK) {
		printf("HAL: TIM2 setPWM error (2)!\n");
	}
}

static void lcd_Delay(__IO uint32_t nCount) {
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

void lcd_Init_Seq_9486(){

	lcd_set_RST_low();
	lcd_Delay(300);
	lcd_set_RST_high();
	lcd_Delay(300);

	lcd_Delay(50);
	// The send data D function stands for a bit of delay, LCD likes it better upon init
	lcd_send_cmd_d(0xF1); //exit sleep

	lcd_send_cmd_d(0x3A); // interface pixel format
	lcd_send_data(0x55); // 16 bit pixel

	lcd_send_cmd_d(0xC2); // Power Control 3
	lcd_send_data(0x22);

	// positive gamma
	lcd_send_cmd_d(0xE0);
		// defaut conf
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

	lcd_send_cmd_d(0x36);
		lcd_send_data(0x48);

	lcd_send_cmd_d(0x11);                     // sleep out
		lcd_Delay(150);

		lcd_send_cmd_d(0x29);                     // display on
		lcd_Delay(150);
}

void lcd_Init_Seq_9481_b(){

	lcd_set_RST_low();
	lcd_Delay(300);
	lcd_set_RST_high();
	lcd_Delay(300);

	lcd_send_cmd_d(0x11);
	lcd_Delay(20);
	lcd_send_cmd_d(0xD0);
	lcd_send_data_d(0x07);
	lcd_send_data_d(0x42);
	lcd_send_data_d(0x18);

	lcd_send_cmd_d(0xD1);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x07);
	lcd_send_data_d(0x10);

	lcd_send_cmd_d(0xD2);
	lcd_send_data_d(0x01);
	lcd_send_data_d(0x02);

	lcd_send_cmd_d(0xC0);
	lcd_send_data_d(0x10);
	lcd_send_data_d(0x3B);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x02);
	lcd_send_data_d(0x11);

	lcd_send_cmd_d(0xC5);
	lcd_send_data_d(0x03);

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

	lcd_send_cmd_d(0x36);
	lcd_send_data_d(0x0A);

	lcd_send_cmd_d(0x3A);
	lcd_send_data_d(0x55);

	lcd_send_cmd_d(0x2A);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x01);
	lcd_send_data_d(0x3F);

	lcd_send_cmd_d(0x2B);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x00);
	lcd_send_data_d(0x01);
	lcd_send_data_d(0xE0);

	lcd_send_cmd_d(0x21);

	lcd_Delay(120);
	lcd_send_cmd_d(0x29);
}

void lcd_Init_Seq_9488(){

	lcd_set_RST_low();
	lcd_Delay(300);
	lcd_set_RST_high();
	lcd_Delay(300);

	lcd_Delay(50);
  lcd_send_cmd_d(0xE0);
	lcd_send_data(0x00);
	lcd_send_data(0x03);
	lcd_send_data(0x09);
	lcd_send_data(0x08);
	lcd_send_data(0x16);
	lcd_send_data(0x0A);
	lcd_send_data(0x3F);
	lcd_send_data(0x78);
	lcd_send_data(0x4C);
	lcd_send_data(0x09);
	lcd_send_data(0x0A);
	lcd_send_data(0x08);
	lcd_send_data(0x16);
	lcd_send_data(0x1A);
	lcd_send_data(0x0F);


	lcd_send_cmd_d(0XE1);
	lcd_send_data(0x00);
	lcd_send_data(0x16);
	lcd_send_data(0x19);
	lcd_send_data(0x03);
	lcd_send_data(0x0F);
	lcd_send_data(0x05);
	lcd_send_data(0x32);
	lcd_send_data(0x45);
	lcd_send_data(0x46);
	lcd_send_data(0x04);
	lcd_send_data(0x0E);
	lcd_send_data(0x0D);
	lcd_send_data(0x35);
	lcd_send_data(0x37);
	lcd_send_data(0x0F);

	lcd_send_cmd_d(0XC0);      //Power Control 1
	lcd_send_data(0x17);    //Vreg1out
	lcd_send_data(0x15);    //Verg2out

	lcd_send_cmd_d(0xC1);      //Power Control 2
	lcd_send_data(0x41);    //VGH,VGL

	lcd_send_cmd_d(0xC5);      //Power Control 3
	lcd_send_data(0x00);
	lcd_send_data(0x12);    //Vcom
	lcd_send_data(0x80);

	lcd_send_cmd_d(0x36);      //Memory Access
	lcd_send_data(0x48);

	lcd_send_cmd_d(0x3A);      // Interface Pixel Format
	lcd_send_data(0x55);

	lcd_send_cmd_d(0x11);                     // sleep out
	lcd_Delay(150);

	lcd_send_cmd_d(0x29);                     // display on
	lcd_Delay(150);
}

// ILI9481
void lcd_Init_Seq_9481(){
	//reset
	lcd_set_RST_low();
	lcd_Delay(300);
	lcd_set_RST_high();
	lcd_Delay(300);

	LCD_Write_COM(0x11);
	lcd_Delay(20);
	LCD_Write_COM(0xD0);
	LCD_Write_DATA(0x07);
	LCD_Write_DATA(0x42);
	LCD_Write_DATA(0x18);

	LCD_Write_COM(0xD1);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x07);
	LCD_Write_DATA(0x10);

	LCD_Write_COM(0xD2);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x02);

	LCD_Write_COM(0xC0);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x3B);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x11);

	LCD_Write_COM(0xC5);
	LCD_Write_DATA(0x03);

	LCD_Write_COM(0xC8);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x32);
	LCD_Write_DATA(0x36);
	LCD_Write_DATA(0x45);
	LCD_Write_DATA(0x06);
	LCD_Write_DATA(0x16);
	LCD_Write_DATA(0x37);
	LCD_Write_DATA(0x75);
	LCD_Write_DATA(0x77);
	LCD_Write_DATA(0x54);
	LCD_Write_DATA(0x0C);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0x36);
	LCD_Write_DATA(0x0A);

	LCD_Write_COM(0x3A);
	LCD_Write_DATA(0x55);

	LCD_Write_COM(0x2A);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x3F);

	LCD_Write_COM(0x2B);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0xE0);
	lcd_Delay(120);
	LCD_Write_COM(0x29);
	LCD_Write_COM(0x2C);
	//lcd_set_RD(0);
}

static void lcd_send_data(uint8_t data){
	lcd_set_RD_high();
	lcd_set_RS_high();
	lcd_set_CS_low();

	LCD_DAT_PORT->ODR &= 0xFF00;
	LCD_DAT_PORT->ODR |= data;

	lcd_set_WR_low();
#ifdef LCD_TYPE_B
	lcd_Delay(1);
#endif
	lcd_set_WR_high();

	LCD_DAT_PORT->ODR &= 0xFF00;
}

static void lcd_send_data_d(uint8_t data){
	lcd_set_RD_high();
	lcd_set_RS_high();
	lcd_set_CS_low();

	LCD_DAT_PORT->ODR &= 0xFF00;
	LCD_DAT_PORT->ODR |= data;

	lcd_set_WR_low();
#ifdef LCD_TYPE_B
	lcd_Delay(30);
#else
	lcd_Delay(5);
#endif
	lcd_set_WR_high();

	LCD_DAT_PORT->ODR &= 0xFF00;
}

static void lcd_send_cmd(uint8_t data) {
	lcd_set_RD_high();
	lcd_set_RS_low();
	lcd_set_CS_low();

	LCD_DAT_PORT->ODR &= 0xFF00;
	LCD_DAT_PORT->ODR |= data;

	lcd_set_WR_low();
	lcd_Delay(5);
	lcd_set_WR_high();
	lcd_set_CS_high();
	LCD_DAT_PORT->ODR &= 0xFF00;
}

void lcd_send_cmd_d(uint8_t data) {
	lcd_set_RD_high();
	lcd_set_RS_low();
	lcd_set_CS_low();

	LCD_DAT_PORT->ODR &= 0xFF00;
	LCD_DAT_PORT->ODR |= data;

	lcd_set_WR_low();
	lcd_Delay(120);
	lcd_set_WR_high();
	lcd_set_CS_high();
	LCD_DAT_PORT->ODR &= 0xFF00;
}


void lcd_hw_set_xy(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
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

void lcd_hw_set_cursor(uint16_t Xpos, uint16_t Ypos) {
	lcd_send_cmd(0x2A);
	lcd_send_data(Xpos >> 8);
	lcd_send_data(0x00FF & Xpos);
	lcd_send_cmd(0x2B);
	lcd_send_data(Ypos >> 8);
	lcd_send_data(0x00FF & Ypos);
	lcd_send_cmd(0x2C);
}

void lcd_hw_Draw_Point(uint16_t color) {
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

void lcd_GPIO_Init() {
	GPIO_InitTypeDef GPIO_InitStructure;

	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();

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

	lcd_init_pin(LCD_RST_PORT, LCD_RST_PIN);
	lcd_init_pin(LCD_CS_PORT, LCD_CS_PIN);
	lcd_init_pin(LCD_RS_PORT, LCD_RS_PIN);
	lcd_init_pin(LCD_WR_PORT, LCD_WR_PIN);
	lcd_init_pin(LCD_RD_PORT, LCD_RD_PIN);
}
