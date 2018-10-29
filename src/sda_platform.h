#ifndef SDA_PLATFORM_H
#define SDA_PLATFORM_H

#define SDA_BASE_SYSLED_PORT GPIOB
#define SDA_BASE_SYSLED_PIN GPIO_PIN_2

#define SDA_BASE_SPEAKER_PORT GPIOA
#define SDA_BASE_SPEAKER_PIN GPIO_PIN_4

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
#include "SDA_OS/SDA_OS.h"

void sda_platform_gpio_init();

#endif
