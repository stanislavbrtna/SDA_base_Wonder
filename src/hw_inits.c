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

	platform_gpio_ain(GPIOE, GPIO_PIN_7|GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_11);

	platform_gpio_ain(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15);
}
