#include "sda_platform.h"

RNG_HandleTypeDef rng;

void rrand_init() {
	__HAL_RCC_RNG_CLK_ENABLE ();
	rng.Instance = RNG;
	if(HAL_RNG_Init(&rng) != HAL_OK) {
		printf("HAL: RNG init error!\n");
	}
}

static void platform_gpio_ain(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin   = pin;
	GPIO_InitStructure.Mode  = GPIO_MODE_ANALOG;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(port, &GPIO_InitStructure);
}

static void platform_gpio_out(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.Pin   = pin;
	GPIO_InitStructure.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull  = GPIO_PULLUP;
	HAL_GPIO_Init(port, &GPIO_InitStructure);
}

static void platform_gpio_in(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef GPIO_InitStructure;
	HAL_GPIO_DeInit(port, pin);
	GPIO_InitStructure.Pin = pin;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(port, &GPIO_InitStructure);
}

void sda_platform_gpio_init() {
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();

	//ledpin
	platform_gpio_out(GPIOB, GPIO_PIN_2);
	sda_set_led(1);

	//PA4 spkr
	platform_gpio_out(GPIOA, GPIO_PIN_4);

	//PA0 in - sw-on
	platform_gpio_in(GPIOA, GPIO_PIN_0);

	// btn A, B, left, right, down, up
	platform_gpio_in(GPIOE, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4|GPIO_PIN_5);

	//PB1 - USB PowerDetect
	platform_gpio_in(GPIOB, GPIO_PIN_1);

	// unused pins are set as AIN to save power
	platform_gpio_ain(GPIOD, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_3|GPIO_PIN_4);
	// internal expansion is not used by default
	platform_gpio_ain(GPIOD, GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

	platform_gpio_ain(GPIOE, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14);

	platform_gpio_ain(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);

}


static void sda_internal_pin_def_hw(GPIO_TypeDef *port, uint32_t pin, uint8_t pinType, uint8_t pull) {
	GPIO_InitTypeDef GPIO_InitStructure;
	HAL_GPIO_DeInit(port, pin);
	GPIO_InitStructure.Pin = pin;
	// some defaults so it does not do werid stuff
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	GPIO_InitStructure.Pull = GPIO_NOPULL;

	if (pinType == SDA_BASE_PIN_IN) {
		GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	}

	if (pinType == SDA_BASE_PIN_OUT) {
		GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	}

	if (pinType == SDA_BASE_PIN_ALT) {
		if ((pin != 8 || pin != 8) && (port != GPIOD)){
			printf("alternate init error\n");
			return;
		}
		if ((pin != 10 || pin != 11) && (port != GPIOB)){
			printf("alternate init error\n");
			return;
		}
		if ((pin != 1) && (port != GPIOA)){
			// we will init the pin
			return;
		}
		GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStructure.Pull = GPIO_PULLUP;
		GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
		GPIO_InitStructure.Alternate = GPIO_AF7_USART3;
		HAL_GPIO_Init(port, &GPIO_InitStructure);
		return;
	}

	if (pull == SDA_BASE_PIN_NOPULL) {
		GPIO_InitStructure.Pull = GPIO_NOPULL;
	}

	if (pull == SDA_BASE_PIN_PULLDOWN) {
		GPIO_InitStructure.Pull = GPIO_PULLDOWN;
	}

	if (pull == SDA_BASE_PIN_PULLUP) {
		GPIO_InitStructure.Pull = GPIO_PULLUP;
	}

	HAL_GPIO_Init(port, &GPIO_InitStructure);
}

void sda_internal_pin_def(uint8_t pinNum, uint8_t pinType, uint8_t pull) {
	switch(pinNum) {
		case 1  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_15, pinType, pull);
			break;
		case 2  :
			sda_internal_pin_def_hw(GPIOB, GPIO_PIN_15, pinType, pull);
			break;
		case 3  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_14, pinType, pull);
			break;
		case 4  :
			sda_internal_pin_def_hw(GPIOB, GPIO_PIN_14, pinType, pull);
			break;
		case 5  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_13, pinType, pull);
			break;
		case 6  :
			sda_internal_pin_def_hw(GPIOB, GPIO_PIN_13, pinType, pull);
			break;
		case 7  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_12, pinType, pull);
			break;
		case 9  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_11, pinType, pull);
			break;
		case 11  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_10, pinType, pull);
			break;
		case 12  :
			sda_internal_pin_def_hw(GPIOE, GPIO_PIN_12, pinType, pull);
			break;
		case 13  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_9, pinType, pull);
			break;
		case 14  :
			sda_internal_pin_def_hw(GPIOE, GPIO_PIN_13, pinType, pull);
			break;
		case 15  :
			sda_internal_pin_def_hw(GPIOD, GPIO_PIN_8, pinType, pull);
			break;
		case 16  :
			sda_internal_pin_def_hw(GPIOE, GPIO_PIN_14, pinType, pull);
			break;

		default :
			printf("sda_internal_pin_def: Unknown pin\n");
	}
}

void sda_internal_pin_set(uint8_t pinNum, uint8_t val) {
	GPIO_PinState pinval = GPIO_PIN_RESET;

	if (val) {
		pinval = GPIO_PIN_SET;
	}

	switch(pinNum) {
		case 1  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, pinval);
			break;
		case 2  :
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, pinval);
			break;
		case 3  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, pinval);
			break;
		case 4  :
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, pinval);
			break;
		case 5  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, pinval);
			break;
		case 6  :
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, pinval);
			break;
		case 7  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, pinval);
			break;
		case 9  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, pinval);
			break;
		case 11  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, pinval);
			break;
		case 12  :
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, pinval);
			break;
		case 13  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, pinval);
			break;
		case 14  :
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, pinval);
			break;
		case 15  :
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, pinval);
			break;
		case 16  :
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, pinval);
			break;

		default :
			printf("sda_internal_pin_set: Unknown pin\n");
	}
}

uint8_t sda_internal_pin_get(uint8_t pinNum) {
	GPIO_PinState pinval = GPIO_PIN_RESET;

	switch(pinNum) {
		case 1  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_15);
			break;
		case 2  :
			pinval = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_15);
			break;
		case 3  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_14);
			break;
		case 4  :
			pinval = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_14);
			break;
		case 5  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_13);
			break;
		case 6  :
			pinval = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
			break;
		case 7  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_12);
			break;
		case 9  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_11);
			break;
		case 11  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_10);
			break;
		case 12  :
			pinval = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_12);
			break;
		case 13  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_9);
			break;
		case 14  :
			pinval = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_13);
			break;
		case 15  :
			pinval = HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_8);
			break;
		case 16  :
			pinval = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_14);
			break;

		default :
			printf("sda_internal_pin_get: Unknown pin\n");
	}
	if (pinval == GPIO_PIN_SET) {
		return 1;
	} else {
		return 0;
	}
}


void sda_external_pin_def(uint8_t pinNum, uint8_t pinType, uint8_t pull) {
	switch(pinNum) {
		case 2  :
			sda_internal_pin_def_hw(GPIOA, GPIO_PIN_1, pinType, pull);
			break;
		case 4  :
			sda_internal_pin_def_hw(GPIOE, GPIO_PIN_15, pinType, pull);
			break;
		case 5  :
			sda_internal_pin_def_hw(GPIOB, GPIO_PIN_10, pinType, pull);
			break;
		case 6  :
			sda_internal_pin_def_hw(GPIOB, GPIO_PIN_11, pinType, pull);
			break;

		default :
			printf("sda_external_pin_def: Unknown pin\n");
	}
}

void sda_external_pin_set(uint8_t pinNum, uint8_t val) {
	GPIO_PinState pinval = GPIO_PIN_RESET;

	if (val) {
		pinval = GPIO_PIN_SET;
	}

	switch(pinNum) {
		case 2  :
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, pinval);
			break;
		case 4  :
			HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, pinval);
			break;
		case 5  :
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, pinval);
			break;
		case 6  :
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, pinval);
			break;

		default :
			printf("sda_external_pin_set: Unknown pin\n");
	}
}

uint8_t sda_external_pin_get(uint8_t pinNum) {
	GPIO_PinState pinval = GPIO_PIN_RESET;

	switch(pinNum) {

		case 2  :
			pinval = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
			break;
		case 4  :
			pinval = HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_15);
			break;
		case 5  :
			pinval = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_10);
			break;
		case 6  :
			pinval = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
			break;

		default :
			printf("sda_internal_pin_get: Unknown pin\n");
	}
	if (pinval == GPIO_PIN_SET) {
		return 1;
	} else {
		return 0;
	}
}

extern ADC_HandleTypeDef g_AdcHandle;
extern ADC_HandleTypeDef g_AdcHandle2;
extern volatile float ADC_Measurement_const;

float sda_external_ADC_get() {
	ADC_ChannelConfTypeDef adcChannel;
	GPIO_InitTypeDef gpioInit;

	irq_lock = 1;

	__GPIOB_CLK_ENABLE();
	__GPIOA_CLK_ENABLE();
	__ADC1_CLK_ENABLE();
	__ADC2_CLK_ENABLE();

	// Deinit the ADCs, init measurement setup,
	// measure, then init everything back
	g_AdcHandle2.Instance = ADC2;
	HAL_ADC_DeInit(&g_AdcHandle2);
	uint16_t measuredVoltage = 0;

	g_AdcHandle2.Instance                   = ADC2;
	g_AdcHandle2.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
	g_AdcHandle2.Init.Resolution            = ADC_RESOLUTION_12B;
	g_AdcHandle2.Init.ScanConvMode          = ENABLE;
	g_AdcHandle2.Init.ContinuousConvMode    = ENABLE;
	g_AdcHandle2.Init.DiscontinuousConvMode = DISABLE;
	g_AdcHandle2.Init.NbrOfDiscConversion   = 0;
	g_AdcHandle2.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	g_AdcHandle2.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
	g_AdcHandle2.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	g_AdcHandle2.Init.NbrOfConversion       = 1;
	g_AdcHandle2.Init.DMAContinuousRequests = ENABLE;
	g_AdcHandle2.Init.EOCSelection          = DISABLE;

	//ADC2 in 8 - batt
	adcChannel.Channel = ADC_CHANNEL_1;
	adcChannel.Rank    = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;

	if(HAL_ADC_Init(&g_AdcHandle2) != HAL_OK) {
		printf("external adc: hal init FAIL\n");
	}

	if (HAL_ADC_ConfigChannel(&g_AdcHandle2, &adcChannel) != HAL_OK) {
		printf("batt_dbg: hal channel 1 init FAIL\n");
	}

	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_7);

	//PA1 ain
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_1);
	gpioInit.Pin   = GPIO_PIN_1;
	gpioInit.Mode  = GPIO_MODE_ANALOG;
	gpioInit.Pull  = GPIO_NOPULL;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &gpioInit);

	HAL_ADC_Start(&g_AdcHandle2);
	if (HAL_ADC_PollForConversion(&g_AdcHandle2, 1000000) == HAL_OK) {
		measuredVoltage = HAL_ADC_GetValue(&g_AdcHandle2);
	}
	HAL_ADC_Stop(&g_AdcHandle2);

	g_AdcHandle.Instance = ADC1;

	HAL_ADC_DeInit(&g_AdcHandle);
	HAL_ADC_DeInit(&g_AdcHandle2);

	touchAdcInitExt();

	irq_lock = 0;
	return (float)measuredVoltage * ADC_Measurement_const;
}
