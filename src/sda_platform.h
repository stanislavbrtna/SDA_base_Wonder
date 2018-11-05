#ifndef SDA_PLATFORM_H
#define SDA_PLATFORM_H

#define SDA_BASE_SYSLED_PORT GPIOB
#define SDA_BASE_SYSLED_PIN GPIO_PIN_2

#define SDA_BASE_SPEAKER_PORT GPIOA
#define SDA_BASE_SPEAKER_PIN GPIO_PIN_4

#define SDA_BASE_BTN_A_PIN GPIO_PIN_0
#define SDA_BASE_BTN_A_PORT GPIOE

#define SDA_BASE_BTN_LEFT_PIN GPIO_PIN_2
#define SDA_BASE_BTN_LEFT_PORT GPIOE

#define SDA_BASE_BTN_UP_PIN GPIO_PIN_5
#define SDA_BASE_BTN_UP_PORT GPIOE

#define SDA_BASE_BTN_DOWN_PIN GPIO_PIN_4
#define SDA_BASE_BTN_DOWN_PORT GPIOE

#define SDA_BASE_BTN_RIGHT_PIN GPIO_PIN_3
#define SDA_BASE_BTN_RIGHT_PORT GPIOE

#define SDA_BASE_BTN_B_PIN GPIO_PIN_1
#define SDA_BASE_BTN_B_PORT GPIOE

#define SDA_BASE_PIN_IN 0
#define SDA_BASE_PIN_OUT 1
#define SDA_BASE_PIN_ALT 2

#define SDA_BASE_PIN_NOPULL 0
#define SDA_BASE_PIN_PULLDOWN 1
#define SDA_BASE_PIN_PULLUP 1

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_cortex.h"
#include "FATFS/ff.h"
#include "sda_fs_umc.h"
#include "DRIVERS/lcd.h"
#include "DRIVERS/touch.h"
#include "DRIVERS/rtc.h"
#include "DRIVERS/usart.h"
#include "DRIVERS/usart3.h"
#include "DRIVERS/speaker.h"
#include "SDA_OS/SDA_OS.h"

void sda_platform_gpio_init();

#endif
