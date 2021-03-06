#include "usart.h"

extern UART_HandleTypeDef huart2;
extern uint8_t sdaDbgSerialEnabled;
extern volatile sdaLockState tick_lock;

void MX_USART2_UART_Init(void) {

  huart2.Instance = USART2;
  HAL_UART_DeInit (&huart2);

  huart2.Init.BaudRate     = 9600;
  huart2.Init.WordLength   = UART_WORDLENGTH_8B;
  huart2.Init.StopBits     = UART_STOPBITS_1;
  huart2.Init.Parity       = UART_PARITY_NONE;
  huart2.Init.Mode         = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;

  if (HAL_UART_Init(&huart2) != HAL_OK) {
    //Error_Handler();
  }
}

//#define DBG_UART

#ifdef DBG_UART
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance == USART2) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  }
}

void MX_USART2_UART_DeInit(void){
    huart2.Instance = USART2;
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle){
  if(uartHandle->Instance == USART2) {
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
  }
}
#else
void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance == USART2) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    GPIO_InitStruct.Pin   = GPIO_PIN_5 | GPIO_PIN_6;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    GPIO_InitStruct.Pin   = GPIO_PIN_7;
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
		HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

		HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);
  }
}

void MX_USART2_UART_DeInit(void){
    huart2.Instance = USART2;
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5 | GPIO_PIN_6);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle){
  if(uartHandle->Instance == USART2) {
    __HAL_RCC_USART2_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5 | GPIO_PIN_6);
  }
}


void uart2_transmit(uint8_t *str, uint32_t len) {
  if (!sdaDbgSerialEnabled) {
    sda_dbg_serial_enable();
  }

  HAL_UART_Transmit(&huart2, str, len, 1000);
}


uint8_t uart2_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
  static uint8_t buff[512];
  uint32_t tickstart = 0U;

  if (!sdaDbgSerialEnabled) {
    sda_dbg_serial_enable();
  }

  for(uint32_t i = 0; i < sizeof(buff); i++) {
    buff[i] = 0;
  }

  tickstart = HAL_GetTick();

  uint32_t i = 0;
  tick_lock = SDA_LOCK_LOCKED;
  while (i < len) {
    uint8_t c;
    if (HAL_UART_Receive(&huart2, &c, sizeof(c), timeout / len) != HAL_OK) {
      if ( HAL_GetTick() > (tickstart + timeout + 10)) {
        if(i == 0){
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
#endif
