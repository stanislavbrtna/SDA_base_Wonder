/*
Copyright (c) 2024 Stanislav Brtna

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

#define LCD_BL_PRESCALER 512

TIM_HandleTypeDef blTimer;

static void setBacklight(TIM_HandleTypeDef timer, uint32_t channel, uint16_t pulse);

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

  blTimer.Instance               = LCD_BL_TIMER;
  blTimer.Channel                = HAL_TIM_ACTIVE_CHANNEL_2;
  blTimer.Init.Prescaler         = LCD_BL_PRESCALER;
  blTimer.Init.CounterMode       = TIM_COUNTERMODE_UP;
  blTimer.Init.Period            = 256;
  blTimer.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  blTimer.Init.RepetitionCounter = 0;

  if(HAL_TIM_PWM_Init(&blTimer) != HAL_OK) {
    printf("HAL: TIM2 init error!\n");
  }
  
  setBacklight(blTimer, LCD_BL_CHANNEL, 128);
}

void lcd_bl_timer_OC_update() {
  HAL_TIM_PWM_Stop(&blTimer, LCD_BL_CHANNEL);
  HAL_TIM_PWM_DeInit(&blTimer);
  // stop generation of pwm
  blTimer.Init.Prescaler = LCD_BL_PRESCALER;
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
