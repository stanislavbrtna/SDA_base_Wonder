/**
  ******************************************************************************
  * @file    sd_diskio.c
  * @author  MCD Application Team
  * @version V1.4.0
  * @date    09-September-2016
  * @brief   SD Disk I/O driver
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */ 

#ifndef SDA_WONDER_USE_SPI
/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "diskio.h"
#include "ff.h"
#include "../sda_platform.h"


// simplyfied for use in SDA sw stack

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

SD_HandleTypeDef mainSD;
extern volatile uint8_t cpuClkLowFlag;

void SD_setSpeedHi() {
	if (cpuClkLowFlag) {
		system_clock_set_normal();
	}

	for (int i = 0; i<10000; i++);
}

HAL_StatusTypeDef sda_sdio_hw_init() {
	static uint8_t init;

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	if (init == 0) {
		/* Enable sdio clock */
		__HAL_RCC_SDIO_CLK_ENABLE();

		/* Enable GPIOs clock */
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOD_CLK_ENABLE();

		/* Common GPIO configuration */
		GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull      = GPIO_NOPULL;
		GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
		GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC;

		/* GPIOC configuration */
		GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
		HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

		/* GPIOD configuration */
		GPIO_InitStruct.Pin = GPIO_PIN_2;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);


		mainSD.Instance = SDIO;
		mainSD.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
		mainSD.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
		mainSD.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
		mainSD.Init.BusWide = SDIO_BUS_WIDE_1B;
		mainSD.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
		mainSD.Init.ClockDiv = 10;
	}
	int tries;

	for(tries = 0; tries < 10; ++tries) {
		/* HAL SD initialization */
		if(HAL_SD_Init(&mainSD) != HAL_OK)
		{
				continue;
		}
		//printf("ok:\n");

		init = 1;

		//Enable wide operation
		if(HAL_SD_ConfigWideBusOperation(&mainSD, SDIO_BUS_WIDE_4B) != HAL_OK)
		{
			if (tries > 5){
				printf("going in 1bit mode\n");
				return HAL_OK;
			}	else
			continue;
		}

		/* Everything is ok */

		return HAL_OK;
	}
	init = 2;
	printf("SD init failed\n");
	return HAL_ERROR;
}

/**
  * @brief  Initializes a Drive
  * @param  lun : not used 
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_initialize(BYTE lun) {
	(void)(lun);
  Stat = STA_NOINIT;

  if (sda_sdio_hw_init() == HAL_OK) {
  	//printf("disk_initialize: OK\n");
  	Stat = 0;
  } else {
  	//printf("disk_initialize: FAIL\n");
  }

  return Stat;
}

/**
  * @brief  Gets Disk Status
  * @param  lun : not used
  * @retval DSTATUS: Operation status
  */
DSTATUS disk_status(BYTE lun)
{
	(void)(lun);
  /*Stat = STA_NOINIT;

  if(HAL_SD_GetCardState(&mainSD) == HAL_SD_CARD_TRANSFER)
  {
    Stat &= ~STA_NOINIT;
    printf("stat FAIL\n");
  }
  
  if (Stat != 0) {
  	printf("stat not OK\n");
  }*/

  return Stat;
}

/**
  * @brief  Reads Sector(s)
  * @param  lun : not used
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */ /*
DRESULT disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
  DRESULT res = RES_ERROR;
  uint32_t timeout = 100000;
  uint8_t ok = 0;

  mainSD.ErrorCode = 0;
	if(HAL_SD_ReadBlocks(&mainSD, buff,(uint32_t) (sector), count, 1000000) == HAL_OK) {
		ok = 1;
		//printf("got %u\n", *buff);
		res = RES_OK;
	} else {
		printf ("read failed (%u) (sector: %u, count: %u)\n",mainSD.ErrorCode, sector, count);
	}
  
  return res;
}*/
DRESULT disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	(void)(lun);
    DRESULT  res;
    uint32_t timeout = 0;
    SD_setSpeedHi();
    res = RES_ERROR;
    if(HAL_SD_ReadBlocks(&mainSD, buff,(uint32_t) (sector), count, 10000000) == HAL_OK)
    {
        /* Wait until the read operation is finished */

        while(timeout++ != 10000000)
        {
        		//printf("loop?\n");
            if(HAL_SD_GetCardState(&mainSD) == HAL_SD_CARD_TRANSFER )
            {
                return RES_OK;
            } else {
            	printf ("read failed (%u) (sector: %u, count: %u)\n",(unsigned int)mainSD.ErrorCode, (unsigned int)sector, (unsigned int)count);
            }
        }
    }

    return res;
}

/**
  * @brief  Writes Sector(s)
  * @param  lun : not used
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */

DRESULT disk_write(BYTE lun, const BYTE *buff, DWORD sector, UINT count)
{
	(void)(lun);
  DRESULT res = RES_ERROR;
  uint32_t timeout = 0;
  SD_setSpeedHi();
		if(HAL_SD_WriteBlocks(&mainSD,(uint8_t *) buff,
        (uint32_t)(sector),
        count, 1000000) == HAL_OK) {
			res = RES_OK;

			while(timeout++ != 10000000)
			        {
			        		//printf("loop?\n");
			            if(HAL_SD_GetCardState(&mainSD) == HAL_SD_CARD_TRANSFER )
			            {
			                return RES_OK;
			            }
			        }
		} else {
			printf ("write failed (%u) (sector: %u, count: %u)\n",(unsigned int)mainSD.ErrorCode, (unsigned int)sector, (unsigned int)count);
		}

  return res;
}


/**
  * @brief  I/O control operation
  * @param  lun : not used
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */

DRESULT disk_ioctl(BYTE lun, BYTE cmd, void *buff)
{
	(void)(lun);
  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;
  SD_setSpeedHi();
  
  if (Stat & STA_NOINIT) return RES_NOTRDY;
  
  switch (cmd)
  {
  /* Make sure that no pending write process */
  case CTRL_SYNC :
    res = RES_OK;
    break;
  
  /* Get number of sectors on the disk (DWORD) */
  case GET_SECTOR_COUNT :
  	HAL_SD_GetCardInfo(&mainSD, &CardInfo);
    *(DWORD*)buff = CardInfo.LogBlockNbr;
    res = RES_OK;
    break;
  
  /* Get R/W sector size (WORD) */
  case GET_SECTOR_SIZE :
  	HAL_SD_GetCardInfo(&mainSD, &CardInfo);
    *(WORD*)buff = CardInfo.LogBlockSize;
    res = RES_OK;
    break;
  
  /* Get erase block size in unit of sector (DWORD) */
  case GET_BLOCK_SIZE :
  	HAL_SD_GetCardInfo(&mainSD, &CardInfo);
    *(DWORD*)buff = CardInfo.LogBlockSize;
    break;
  
  default:
    res = RES_PARERR;
  }
  
  return res;
}

DWORD get_fattime()
{
	//int time = RTC_GetCounter();
	//int y, m, d;
	//epoch_days_to_date(time/DAY_SECONDS, &y, &m, &d);
	//time %= DAY_SECONDS;
	//return 0x3a000000;

	return (svpSGlobal.year-1980)<<25 | svpSGlobal.month<<21 | svpSGlobal.day<<16 | \
				(svpSGlobal.hour)<<11 | (svpSGlobal.min)<<5 | (svpSGlobal.sec/2%30);

}

#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

