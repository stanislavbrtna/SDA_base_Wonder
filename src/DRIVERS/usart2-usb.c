#include "usart2-usb.h"

extern UART_HandleTypeDef huart2;
extern uint8_t sdaUsbSerialEnabled;
extern volatile sdaLockState tick_lock;

uint32_t uart2BaudRate;

void MX_USART2_UART_Init(void) {

  huart2.Instance = USART2;
  HAL_UART_DeInit(&huart2);

  if (uart2BaudRate == 0) {
    uart2BaudRate = 9600;
  }

  huart2.Init.BaudRate     = uart2BaudRate;
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

		HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(USART2_IRQn);
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
#endif

void uart2_quick_init() {
  HAL_UART_MspInit(&huart2);
  MX_USART2_UART_Init();
  sdaUsbSerialEnabled = 1;
}

void uart2_set_default_speed() {
  uart2BaudRate = 9600;
}

void uart2_set_speed(uint32_t bd) {
  uart2BaudRate = bd;
}

void uart2_transmit(uint8_t *str, uint32_t len) {
  if (!sdaUsbSerialEnabled) {
    uart2_quick_init();
  }

  HAL_UART_Transmit(&huart2, str, len, 1000);
}


uint8_t uart2_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
  static uint8_t buff[512];
  uint32_t tickstart = 0U;

  if (!sdaUsbSerialEnabled) {
    uart2_quick_init();
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


uint8_t usart2_buff[512];
volatile uint16_t usart2_buff_n;
volatile uint8_t usart2_c[10];
volatile uint8_t usart2_DR;

uint8_t uart2_recieve_IT() {

  usart2_c[1] = 0;
  usart2_DR = 0;
  usart2_buff_n = 0;

  if (!sdaUsbSerialEnabled) {
    uart2_quick_init();
  }

  for(uint32_t i = 0; i < sizeof(usart2_buff); i++) {
    usart2_buff[i] = 0;
  }

  HAL_StatusTypeDef h;
  h = HAL_UART_Receive_IT(&huart2, usart2_c, 1);

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


uint8_t uart2_get_rdy() {
  return usart2_DR;
}


uint16_t uart2_get_str(uint8_t *str) {
  uint16_t r = 0;
  sdaLockState l;
  if (usart2_DR) {
    l = tick_lock;
    tick_lock = SDA_LOCK_LOCKED;
    HAL_NVIC_DisableIRQ(USART2_IRQn);
    for(uint32_t i = 0; i < sizeof(usart2_buff); i++) {
      str[i] = usart2_buff[i];
    }

    r = usart2_buff_n;

    usart2_DR = 0;
    usart2_buff_n = 0;
    usart2_c[0] = 0;
    for(uint32_t i = 0; i < sizeof(usart2_buff); i++) {
      usart2_buff[i] = 0;
    }
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    tick_lock = l;
    return r;
  } else {
    return 0;
  }
}


void USART2_IRQHandler(void) {
  HAL_UART_IRQHandler(&huart2);
}
