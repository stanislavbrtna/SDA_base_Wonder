#include "sda_platform.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern RNG_HandleTypeDef  rng;

extern uint8_t led_pattern[10];
extern uint16_t led_counter;
extern volatile uint8_t cpuClkLowFlag;
extern volatile sdaLockState tick_lock;

extern volatile uint8_t Lcd_on_flag;
extern volatile uint8_t Lcd_off_flag;

volatile uint8_t sdaSerialEnabled;
volatile uint8_t sdaUsbSerialEnabled;
volatile uint8_t sdaUsbSerialForDebug;

//=============================================================================
// notif LED control

// set notif LED state
void sda_set_led(uint8_t set) {
  if (set) {
    HAL_GPIO_WritePin(SDA_BASE_SYSLED_PORT, SDA_BASE_SYSLED_PIN, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(SDA_BASE_SYSLED_PORT, SDA_BASE_SYSLED_PIN, GPIO_PIN_RESET);
  }
}

// set notif LED pattern
void led_set_pattern(ledPatternType pat) {
  uint8_t x;
  if (pat == LED_ON) {
    for (x = 0; x < 10; x++) {
      led_pattern[x] = 1;
    }
    sda_set_led(1);
  }

  if (pat == LED_OFF) {
    for (x = 0; x < 10; x++) {
      led_pattern[x] = 0;
    }
    sda_set_led(0);
  }

  if (pat == LED_BLINK) {
    led_pattern[0] = 1;
    led_pattern[1] = 1;
    led_pattern[2] = 1;
    led_pattern[3] = 1;
    led_pattern[4] = 1;
    led_pattern[5] = 1;
    led_pattern[6] = 0;
    led_pattern[7] = 0;
    led_pattern[8] = 0;
    led_pattern[9] = 0;
  }

  if (pat == LED_SHORTBLINK) {
    led_pattern[0] = 0;
    led_pattern[1] = 0;
    led_pattern[2] = 0;
    led_pattern[3] = 1;
    led_pattern[4] = 0;
    led_pattern[5] = 0;
    led_pattern[6] = 0;
    led_pattern[7] = 0;
    led_pattern[8] = 0;
    led_pattern[9] = 0;
  }

  if (pat == LED_ALARM) {
    led_pattern[0] = 1;
    led_pattern[1] = 0;
    led_pattern[2] = 1;
    led_pattern[3] = 0;
    led_pattern[4] = 1;
    led_pattern[5] = 0;
    led_pattern[6] = 1;
    led_pattern[7] = 0;
    led_pattern[8] = 1;
    led_pattern[9] = 0;
  }

  led_counter = 0;
}


//=============================================================================
// get if SD is inserted

uint8_t sda_card_inserted() {
  if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_SET) {
    return 0;
  }
  return 1;
}


//=============================================================================
// Expansion serial port (uart3)

void sda_serial_enable() {
  sdaSerialEnabled = 1;
  uart3_set_default_speed();
  HAL_UART3_MspInit(&huart3);
  MX_USART3_UART_Init();
}

void sda_serial_disable() {
  HAL_UART_AbortReceive(&huart3);
  MX_USART3_UART_DeInit();
  sdaSerialEnabled = 0;
}

void sda_serial_init_bd(uint32_t bd) {
  uart3_set_speed(bd);
  MX_USART3_UART_DeInit();
  MX_USART3_UART_Init();
  HAL_UART3_MspInit(&huart3);
  sdaSerialEnabled = 1;
}

uint8_t sda_serial_is_enabled() {
  return sdaSerialEnabled;
}

// blocking functions
uint8_t sda_serial_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
  return uart3_recieve(str, len, timeout);
}

void sda_serial_transmit(uint8_t *str, uint32_t len) {
  uart3_transmit(str, len);
}

// interrupt enabled functions:
uint8_t sda_serial_recieve_init() {
  return uart3_recieve_IT();
}

uint8_t sda_serial_get_rdy() {
  return uart3_get_rdy();
}

uint16_t sda_serial_get_str(uint8_t *str) {
  return uart3_get_str(str);
}

uint8_t sda_serial_get_spec() {
  return uart3_get_spec();
}


//=============================================================================
// USB serial port (uart2)

void sda_usb_serial_enable() {
  uart2_set_default_speed();
  HAL_UART_MspInit(&huart2);
  MX_USART2_UART_Init();
  sdaUsbSerialEnabled = 1;
}

void sda_usb_serial_init_bd(uint32_t bd) {
  uart2_set_speed(bd);
  MX_USART2_UART_DeInit();
  HAL_UART_MspInit(&huart2);
  MX_USART2_UART_Init();
  sdaUsbSerialEnabled = 1;
}

void sda_usb_serial_disable() {
  sdaUsbSerialEnabled = 0;
  MX_USART2_UART_DeInit();
}

uint8_t sda_usb_serial_is_enabled() {
  return sdaUsbSerialEnabled;
}

uint8_t sda_usb_serial_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
  return uart2_recieve(str, len, timeout);
}

void sda_usb_serial_transmit(uint8_t *str, uint32_t len) {
  uart2_transmit(str, len);
}

// interrupt enabled functions:
uint8_t sda_usb_serial_recieve_init() {
  return uart2_recieve_IT();
}

uint8_t sda_usb_serial_get_rdy() {
  return uart2_get_rdy();
}

uint16_t sda_usb_serial_get_str(uint8_t *str) {
  return uart2_get_str(str);
}

void sda_usb_enable_for_dbg(uint8_t val) {
  sdaUsbSerialForDebug = val;
}

uint8_t sda_usb_get_enable_for_dbg() {
  return sdaUsbSerialForDebug;
}

//=============================================================================
// hw RNG

uint32_t svp_random() {
  uint32_t ret = 0;
  HAL_RNG_GenerateRandomNumber(&rng, &ret);
  return ret/2;
}


//=============================================================================
// LCD on/off, backlight

void svp_set_lcd_state(lcdStateType state) {
  if(state == LCD_ON) {
    lcd_hw_wake();
    Lcd_on_flag  = 200;
    Lcd_off_flag = 0;
  } else if (state == LCD_OFF) {
    lcd_bl_off();
    Lcd_off_flag = 30;
    Lcd_on_flag  = 0;
  }
  svpSGlobal.lcdState = state;
}

void svp_set_backlight(uint8_t val) {
  lcd_hw_set_backlight(val);
}


//=============================================================================
// battery voltage measurement

extern volatile uint8_t batt_measured_flag;
float get_batt_voltage();

uint8_t sda_is_battery_measured() {
  if (batt_measured_flag == 1) {
    batt_measured_flag = 0;
    return 1;
  } else {
    return 0;
  }
}

float sda_get_battery_voltage() {
  return get_batt_voltage();
}


//=============================================================================
// System clock settings

void system_clock_set_low(void) {

  if(cpuClkLowFlag == 1) {
    return;
  }
  // Enable Power Control clock
  __PWR_CLK_ENABLE();

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  HAL_RCC_DeInit();
  //hse config
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = (HSE_VALUE/1000000u);
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV8; /* 168 MHz */ //div2
  RCC_OscInitStruct.PLL.PLLQ = 7; /* To make USB work. */
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)==HAL_ERROR)
    printf("ERROR kdyz menim freq! na 42mhz\n");

  // Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  // clocks dividers
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
    | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;

  // This is expected to work for most large cores.
  // Check and update it for your own configuration.
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  SystemCoreClockUpdate();

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2); //max 144Mhz

  reload_clock_sensitive_stuff();

  cpuClkLowFlag = 1;
}

void system_clock_set_normal(void) {

  if(cpuClkLowFlag == 0) {
    return;
  }

  HAL_RCC_DeInit();
  SystemClock_Config();
  SystemCoreClockUpdate();
  reload_clock_sensitive_stuff();

  cpuClkLowFlag = 0;
}


//=============================================================================
// Error halt function (BSOD)

void svs_hardErrHandler() {
  tick_lock = SDA_LOCK_LOCKED;
  LCD_Fill(LCD_MixColor(255, 0, 0));
  LCD_DrawText_ext(32, 100, 0xFFFF, (uint8_t *)"Hard error occured!\nSDA-os will now reset!");
  Delay(50000000);
  HAL_NVIC_SystemReset();
  while(1);
}


