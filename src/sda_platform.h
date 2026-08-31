#ifndef SDA_PLATFORM_H
#define SDA_PLATFORM_H

// Board rev. enum

typedef enum {UNKNOWN, REV1, REV2B} wonderBoardRevisions;

// misc includes

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_cortex.h"
#include "FATFS/ff.h"
#include "sda_fs_umc.h"
#include "DRIVERS/lcd.h"
#include "DRIVERS/touch.h"
#include "DRIVERS/rtc.h"
#include "DRIVERS/usart2-usb.h"
#include "DRIVERS/usart3.h"
#include "DRIVERS/speaker.h"
#include "DRIVERS/power_management.h"
#include "SDA_OS/SDA_OS.h"
#include "hw_misc.h"
#include "BASE_SHARED/post.h"

void sda_platform_gpio_init();
void SystemClock_Config();
void Delay(__IO uint32_t nCount);

#endif
