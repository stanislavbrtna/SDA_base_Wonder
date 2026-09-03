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


// simplified for use in SDA sw stack

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
		for (int i = 0; i<10000; i++);
	}
}

void SD_Debug_PrintErrors(SD_HandleTypeDef *p_hsd)
{
    uint32_t error = p_hsd->ErrorCode;

    if (error == HAL_SD_ERROR_NONE)
    {
        printf("[SD INFO] No errors detected. Card health is OK.\r\n");
        return;
    }

    printf("[SD ERROR] Error detected! Bitmask: 0x%08X\r\n", (unsigned int)error);

    /* Bitmask evaluation */
    if (error & HAL_SD_ERROR_CMD_CRC_FAIL)       printf(" -> CMD CRC FAIL: Command checksum error.\r\n");
    if (error & HAL_SD_ERROR_DATA_CRC_FAIL)      printf(" -> DATA CRC FAIL: Data checksum error (Check wiring/pull-ups).\r\n");
    if (error & HAL_SD_ERROR_CMD_RSP_TIMEOUT)    printf(" -> CMD TIMEOUT: Card failed to respond to command.\r\n");
    if (error & HAL_SD_ERROR_DATA_TIMEOUT)       printf(" -> DATA TIMEOUT: Read/Write operation timed out.\r\n");
    if (error & HAL_SD_ERROR_TX_UNDERRUN)        printf(" -> TX UNDERRUN: MCU failed to feed FIFO during write operation.\r\n");
    if (error & HAL_SD_ERROR_RX_OVERRUN)         printf(" -> RX OVERRUN: MCU failed to read FIFO in time during read operation.\r\n");
    if (error & HAL_SD_ERROR_ADDR_OUT_OF_RANGE)  printf(" -> ADDR OUT OF RANGE: Requested block address out of bounds.\r\n");
    if (error & HAL_SD_ERROR_REQUEST_NOT_APPLICABLE) printf(" -> NOT APPLICABLE: Command unsupported by this card class.\r\n");
    //if (error & HAL_SD_ERROR_BAD_CURRENT_STATE)  printf(" -> BAD CURRENT STATE: Card in invalid operational state.\r\n");
    if (error & HAL_SD_ERROR_PARAM)              printf(" -> PARAM ERROR: Invalid command parameter.\r\n");
    if (error & SDMMC_ERROR_TIMEOUT)             printf(" -> TIMEOUT ERROR: ???.\r\n");
    if (error &  SDMMC_ERROR_LOCK_UNLOCK_FAILED) printf(" -> LOCK UNLOCK FAILED: Try mount&umount with a pc, might fix that.\r\n");
   
    /* Clear error code register to allow recovery */
    p_hsd->ErrorCode = HAL_SD_ERROR_NONE;
}

void SDIO_IRQHandler(void){
  HAL_SD_IRQHandler(&mainSD);
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

    HAL_NVIC_SetPriority(SDIO_IRQn, 2, 2);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

		mainSD.Instance = SDIO;
		mainSD.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
		mainSD.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
		mainSD.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
		mainSD.Init.BusWide = SDIO_BUS_WIDE_1B;
		mainSD.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
		mainSD.Init.ClockDiv = 2;
	}
  

  HAL_SD_DeInit(&mainSD);

  if(HAL_SD_Init(&mainSD) != HAL_OK) {
	  printf("SD init: init error!\n");
    printf("ErrorCode: %u\n", (unsigned int)mainSD.ErrorCode);
    SD_Debug_PrintErrors(&mainSD);
    return HAL_ERROR;
  }

  if(HAL_SD_ConfigWideBusOperation(&mainSD, SDIO_BUS_WIDE_4B) != HAL_OK) {
    printf("SD init: switch to 4bits error!\n");
    printf("ErrorCode: %u\n", (unsigned int)mainSD.ErrorCode);
    SD_Debug_PrintErrors(&mainSD);
    return HAL_ERROR;     
  }
  
  HAL_SD_CardInfoTypeDef CardInfo;
      
  if(HAL_SD_GetCardInfo(&mainSD, &CardInfo) != HAL_OK) {
    printf("SD init: get card info error!\n");
    printf("ErrorCode: %u\n", (unsigned int)mainSD.ErrorCode);
    SD_Debug_PrintErrors(&mainSD);
    return HAL_ERROR;
  }

  printf("SD Info:\n");
  printf("CardType:    %u\n", CardInfo.CardType);
  printf("CardVersion: %u\n", CardInfo.CardVersion);
  printf("Class:       %u\n", CardInfo.Class);
  printf("BlockNbr:    %u\n", CardInfo.BlockNbr);

  printf("SD init OK\n");
	return HAL_OK;
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
	//Stat = STA_NOINIT;
  /*
   * enabling this leads to performance regression...
  if(HAL_SD_GetCardState(&mainSD) == HAL_SD_CARD_TRANSFER)
  {
    Stat &= ~STA_NOINIT;
    //printf("stat FAIL\n");
  }
  
  //if (Stat != 0) {
  //	printf("stat not OK\n");
  //}
  */

  return Stat;
}


DRESULT SD_wait_for_rdy() {
  uint32_t timeout = 0;

  while(HAL_SD_GetCardState(&mainSD) != HAL_SD_CARD_TRANSFER) {
    timeout++;
    if(timeout > 400000){
      printf ("SD OP timeout: HAL_SD_CARD state:%u\n", HAL_SD_GetCardState(&mainSD));
      return RES_ERROR;
    }
  }

  return RES_OK;
}


DRESULT disk_read(BYTE lun, BYTE *buff, DWORD sector, UINT count)
{
	(void)(lun);

  SD_setSpeedHi();

  __disable_irq();
  
  if(HAL_SD_ReadBlocks(&mainSD, buff,(uint32_t) (sector), count, 100000) != HAL_OK) {
    printf ("read failed (%u) (sector: %u, count: %u, state:%u)\n",(unsigned int)mainSD.ErrorCode, (unsigned int)sector, (unsigned int)count);
    SD_Debug_PrintErrors(&mainSD);
    __enable_irq();
    return RES_ERROR;
  }

  if(SD_wait_for_rdy() != RES_OK) {
    __enable_irq();
    return RES_ERROR;
  }

  __enable_irq();
  return  RES_OK;
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

  SD_setSpeedHi();

  __disable_irq();

  if(HAL_SD_WriteBlocks(&mainSD, buff,(uint32_t) (sector), count, 100000) != HAL_OK) {
    printf ("write failed (%u) (sector: %u, count: %u, state:%u)\n",(unsigned int)mainSD.ErrorCode, (unsigned int)sector, (unsigned int)count);
    SD_Debug_PrintErrors(&mainSD);
    __enable_irq();
    return RES_ERROR;
  }

  if(SD_wait_for_rdy() != RES_OK) {
    __enable_irq();
    return RES_ERROR;
  }
__enable_irq();
  return RES_OK;
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

void sd_wait_for_ready() {
  while(HAL_SD_GetCardState(&mainSD) != HAL_SD_CARD_TRANSFER) {}
}

#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

