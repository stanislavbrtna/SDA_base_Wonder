#include "usart3.h"
//#define USART3_DBG

extern UART_HandleTypeDef huart3;
extern volatile uint8_t sdaSerialEnabled;
extern volatile sdaLockState tick_lock;

void MX_USART3_UART_Init(void) {
  huart3.Instance = USART3;
  HAL_UART_DeInit (&huart3);
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    //Error_Handler();
  }
#ifdef USART3_DBG
  printf("usart3 init ok\n");
#endif
}

void HAL_UART3_MspInit(UART_HandleTypeDef* uartHandle) {
  GPIO_InitTypeDef GPIO_InitStruct;
  (void) (uartHandle);
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
	/* Peripheral clock enable */
	__HAL_RCC_USART3_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	/**USART2 GPIO Configuration
	PB10     ------> USART2_TX
	PB11     ------> USART2_RX
	*/

	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

	GPIO_InitStruct.Pin = GPIO_PIN_10|GPIO_PIN_11;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#ifdef USART3_DBG
	printf("usart3 GPIO init ok\n");

	//HAL_NVIC_SetPriority(USART3_IRQn, 0x0C, 5);
	//HAL_NVIC_EnableIRQ(USART3_IRQn);

	printf("usart3 NVIC init ok\n");
#endif

}

  
void MX_USART3_UART_DeInit(void){
    huart3.Instance = USART3;
    __HAL_RCC_USART3_CLK_DISABLE();
  
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

    /* Peripheral DMA DeInit*/
    //HAL_DMA_DeInit(huart3.hdmatx);
    //HAL_DMA_DeInit(huart3.hdmarx);

}
    
void HAL_UART3_MspDeInit(UART_HandleTypeDef* uartHandle){
  if(uartHandle->Instance == USART3) {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);
  }
}

void uart3_transmit(uint8_t *str, uint32_t len) {

	if (!sdaSerialEnabled) {
		sda_serial_enable();
	}

	//while (HAL_UART_GetState(&huart3) != HAL_UART_STATE_READY && HAL_UART_GetState(&huart3) != HAL_UART_STATE_BUSY_RX);

	HAL_UART_Transmit(&huart3, str, len, 1000);
}

uint8_t uart3_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
	static uint8_t buff[512];
	uint32_t tickstart = 0U;

	if (!sdaSerialEnabled) {
		sda_serial_enable();
	}

	for(uint32_t i = 0; i < sizeof(buff); i++) {
		buff[i] = 0;
	}

	tickstart = HAL_GetTick();

	uint32_t i = 0;
	tick_lock = SDA_LOCK_LOCKED;
	while (i < len) {
		uint8_t c;
		if (HAL_UART_Receive(&huart3, &c, sizeof(c), timeout / len) != HAL_OK) {
			if ( HAL_GetTick() > (tickstart + timeout + 100)) {
				if(i == 0){
					printf("uart3 recv error\n");
					tick_lock = SDA_LOCK_UNLOCKED; // enable tick again!
					return 0;
				}else{
					break;
				}
			}
		}else{
			buff[i] = c;
			i++;
		}

	}

	for(i = 0; i < len; i++) {
		str[i] = buff[i];
	}

	tick_lock = SDA_LOCK_UNLOCKED;

	return 1;
}
