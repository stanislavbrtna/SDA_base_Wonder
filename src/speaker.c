#include "sda_platform.h"

TIM_HandleTypeDef beeptimer;

volatile uint8_t beep_flag;
volatile uint16_t beep_t;
volatile uint32_t beep_scaler;

void sda_beep_setup(uint16_t freq);

// beeper timer
void beep_timer_init() {
	__TIM3_CLK_ENABLE();
	// (TIM_CLOCK / (Prescaler + 1)) / (Period +1)
	beeptimer.Instance = TIM3;
	beeptimer.Channel = HAL_TIM_ACTIVE_CHANNEL_4;
	beeptimer.Init.Prescaler = 20;
	beeptimer.Init.CounterMode = TIM_COUNTERMODE_UP;
	beeptimer.Init.Period = 210;
	beeptimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	beeptimer.Init.RepetitionCounter = 0;

	if(HAL_TIM_Base_Init(&beeptimer) != HAL_OK) {
		printf("HAL: TIM3 init error!\n");
	}

	if(HAL_TIM_Base_Stop_IT(&beeptimer) != HAL_OK) {
		printf("HAL: TIM3 start error!\n");
	}

	HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);
	svp_beep_set_def();
}

void TIM3_IRQHandler() {
	HAL_TIM_IRQHandler(&beeptimer);
	SDA_BASE_SPEAKER_PORT->ODR ^= SDA_BASE_SPEAKER_PIN;
}

void svp_beep() {
	if (svpSGlobal.mute == 0) {
		beep_flag = 1;
	} else {
		led_set_pattern(LED_ALARM);
	}
}

void svp_beep_set_t(uint16_t time) {
	beep_t = time;
}

void svp_beep_set_pf(uint16_t val) {
	beep_scaler = val;
	TIM3->PSC = 1680000/val;
	//sda_beep_setup(beep_scaler);
}

void svp_beep_set_def() {
	beep_scaler = 4600;
	beep_t = 250;
}

void sda_beep_setup(uint16_t freq) {
	__TIM3_CLK_ENABLE();
	HAL_TIM_Base_DeInit(&beeptimer);
	printf("clock: %u\n", SystemCoreClock);
	// Prescaler = TIM_CLOCK / ( hz*(Period +1) )
	beeptimer.Instance = TIM3;
	beeptimer.Channel = HAL_TIM_ACTIVE_CHANNEL_4;
	beeptimer.Init.Prescaler = 1680000/freq;
	beeptimer.Init.CounterMode = TIM_COUNTERMODE_UP;
	beeptimer.Init.Period = 200;
	beeptimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	beeptimer.Init.RepetitionCounter = 0;

	if(HAL_TIM_Base_Init(&beeptimer) != HAL_OK) {
		printf("HAL: TIM3 init error!\n");
	}

	if(HAL_TIM_Base_Stop_IT(&beeptimer) != HAL_OK) {
		printf("HAL: TIM3 start error!\n");
	}

	HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);
}

void sda_base_beep_start() {
	sda_beep_setup(beep_scaler);
	HAL_TIM_Base_Start_IT(&beeptimer);
}

