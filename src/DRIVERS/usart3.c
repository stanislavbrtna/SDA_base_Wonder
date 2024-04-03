#include "usart3.h"
//#define USART3_DBG

extern UART_HandleTypeDef huart3;
extern volatile uint8_t sdaSerialEnabled;
extern volatile sdaLockState tick_lock;
static volatile uint8_t it_rcv_enabled;

uint32_t uart3BaudRate;

void MX_USART3_UART_Init(void) {
  huart3.Instance = USART3;
  HAL_UART_DeInit(&huart3);
  huart3.Init.BaudRate = uart3BaudRate;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;


  if (HAL_UART_Init(&huart3) != HAL_OK) {
    printf("Usart3 MX init error!!\n");
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

	HAL_NVIC_SetPriority(USART3_IRQn, 1, 1);
	NVIC_ClearPendingIRQ(USART3_IRQn); // without this the thing borks...
	HAL_NVIC_EnableIRQ(USART3_IRQn);
}

  
void MX_USART3_UART_DeInit(void){
    huart3.Instance = USART3;
    __HAL_RCC_USART3_CLK_DISABLE();
  
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);

}
    
void HAL_UART3_MspDeInit(UART_HandleTypeDef* uartHandle){
  if(uartHandle->Instance == USART3) {
    __HAL_RCC_USART3_CLK_DISABLE();
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10|GPIO_PIN_11);
  }
}

void uart3_wake_up() {
  if (!sdaSerialEnabled) {
    return;
  }

  HAL_UART3_MspInit(&huart3);
  MX_USART3_UART_Init();

  if(it_rcv_enabled) {
    uart3_recieve_IT();
  }

}

static uint8_t usart3_buff[512];
static uint16_t usart3_buff_n;
static uint8_t usart3_c;
static volatile uint8_t usart3_DR;

void uart3_sleep() {
  if(it_rcv_enabled) {
    HAL_UART_AbortReceive_IT(&huart3);
    NVIC_ClearPendingIRQ(USART3_IRQn);
    HAL_NVIC_DisableIRQ(USART3_IRQn);
    usart3_buff_n = 0;
    usart3_c  = 0;
    usart3_DR = 0;
  }
}

void uart3_set_default_speed() {
  uart3BaudRate = 9600;
}

void uart3_set_speed(uint32_t bd) {
  uart3BaudRate = bd;
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

	HAL_UART_AbortReceive_IT(&huart3);
	it_rcv_enabled = 0;

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


/* IT receive API, usage:

  uint8_t buff[512];

  uart3_recieve_IT();

  while(1) {
    if (uart3_get_rdy() == 2) {
      uart3_get_str(&buff);
      printf("got: %s\n", buff);
    } else {
      //printf("none\n");
    }
  }
*/


uint8_t uart3_recieve_IT() {
  usart3_c = 0;
  usart3_DR = 0;
  usart3_buff_n = 0;


  if (!sdaSerialEnabled) {
    sda_serial_enable();
  }

  HAL_UART_AbortReceive_IT(&huart3);
  NVIC_ClearPendingIRQ(USART3_IRQn);
  it_rcv_enabled = 1;

  for(uint32_t i = 0; i < sizeof(usart3_buff); i++) {
    usart3_buff[i] = 0;
  }
  HAL_StatusTypeDef h;
  h = HAL_UART_Receive_IT(&huart3, &usart3_c, 1);

  if(h == HAL_ERROR) {
    printf("serial rcv init error\n");
    return 0;
  }

  if(h == HAL_BUSY) {
    printf("serial rcv init error busy\n");
    return 0;
  }

  tick_lock = SDA_LOCK_UNLOCKED;
  return 1;
}


uint8_t uart3_get_rdy() {
  return usart3_DR;
}


uint16_t uart3_get_str(uint8_t *str) {
  uint16_t r = 0;
  sdaLockState l;
  if (usart3_DR) {
    l = tick_lock;

    //printf("uart3_get_str dbg n: %u, dr: %u, buffer: %s\n", usart3_buff_n, usart3_DR, usart3_buff);

    tick_lock = SDA_LOCK_LOCKED;
    while(NVIC_GetPendingIRQ(USART3_IRQn));

    HAL_NVIC_DisableIRQ(USART3_IRQn);
    uint32_t i;
    for(i = 0; i < (uint32_t)(usart3_buff_n); i++) {
      str[i] = usart3_buff[i];
    }
    str[i] = 0;

    r = usart3_buff_n;

    usart3_DR = 0;
    usart3_buff_n = 0;
    usart3_c = 0;
    //NVIC_ClearPendingIRQ();
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    tick_lock = l;
    return r;
  } else {
    return 0;
  }
}


void USART3_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart3);
}


extern uint8_t usart2_buff[512];
extern volatile uint16_t usart2_buff_n;
extern volatile uint8_t usart2_c[10];
extern volatile uint8_t usart2_DR;

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART3) {
      usart3_buff[usart3_buff_n] = usart3_c;
      usart3_buff_n++;
      if (usart3_buff_n > (uint16_t)(sizeof(usart3_buff) - 1)) {
        usart3_buff_n = 0;
        //printf("USART OVERRUN!\n");
      }
      if (usart3_c == '\n') {
        usart3_DR = 2;
      } else {
        if (usart3_DR == 0) {
          usart3_DR = 1;
        }
      }

      HAL_StatusTypeDef h;
      // hal uart transmit sometimes locks the uart and timeouts,
      // so it can't continue with receive
      // and the whole thing would bork, old hal libs perhaps? dunno
      __HAL_UNLOCK(huart);
      while(h != HAL_OK) {
        h = HAL_UART_Receive_IT(&huart3, &usart3_c, 1);

        if(h == HAL_ERROR) {
          printf("serial rcv init error\n");
        }

        if(h == HAL_BUSY) {
          printf("serial rcv init error busy(state: %u lock: %u) \n", huart->RxState, huart->Lock);
        }
      }
  }

  if (huart->Instance == USART2) {

        usart2_buff[usart2_buff_n] = usart2_c[0];
        usart2_buff_n++;
        if (usart2_buff_n > sizeof(usart2_buff) - 1) {
          usart2_buff_n = 0;
        }
        if (usart2_c[0] == '\n') {
          usart2_DR = 2;
        } else {
          if (usart2_DR == 0) {
            usart2_DR = 1;
          }
        }
      __HAL_UNLOCK(huart); // will do the same here...
      HAL_UART_Receive_IT(&huart2, usart2_c, 1);
    }
}

