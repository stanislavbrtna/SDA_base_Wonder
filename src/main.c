// ----------------------------------------------------------------------------
#include "sda_platform.h"
#include "math.h"

#define MIN_VOLTAGE 3.1
#define MAX_VOLTAGE 4.0
#define BATT_ADC_CONST_DEF 0.0013657
#define VOLTAGE_REF_VAL_DEF 0.626


// hal handles
ADC_HandleTypeDef g_AdcHandle;
ADC_HandleTypeDef g_AdcHandle2;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
extern RNG_HandleTypeDef rng;
/*============================globals========================================*/

FATFS FatFs;
svpStatusStruct svpSGlobal;

volatile wonderBoardRevisions boardRev;

uint32_t uptimeSleepStart;

// systick globals
volatile uint8_t touch_lock;
volatile uint8_t redraw_lock;
volatile uint8_t irq_lock;
volatile uint32_t counter;
volatile uint16_t svsCounter;
volatile uint16_t svsLoadCounter;

volatile uint8_t serialBuf[16];

volatile uint8_t tickLock;
volatile uint8_t Lcd_on_flag;
volatile uint8_t Lcd_off_flag;

volatile uint32_t backlight_scaler;

volatile uint8_t sdaSerialEnabled;
volatile uint8_t sdaDbgSerialEnabled;


extern uint8_t beep_flag;
// led pattern array
uint8_t led_pattern[10];
uint16_t led_counter;

// battery measurement
uint16_t batt_array[60];
volatile uint32_t batt_val;
volatile uint32_t voltage_ref_val;
float batt_adc_const;

volatile uint8_t cpuClkLowFlag;

volatile float ADC_Measurement_const;

void SystemClock_Config(void);
void __initialize_hardware(void);
/*============================Local headers==================================*/
void Delay(__IO uint32_t nCount);
void beep_timer_init();
void rrand_init();
/*============================Delay==========================================*/

void Delay(__IO uint32_t nCount) {
  for(; nCount != 0; nCount--);
}

void sda_set_led(uint8_t set) {
	if (set) {
		HAL_GPIO_WritePin(SDA_BASE_SYSLED_PORT, SDA_BASE_SYSLED_PIN, GPIO_PIN_SET);
	} else {
		HAL_GPIO_WritePin(SDA_BASE_SYSLED_PORT, SDA_BASE_SYSLED_PIN, GPIO_PIN_RESET);
	}
}

/*============================SDA API functions==============================*/
//TODO: fix expansion serial port

uint8_t sda_serial_recieve(uint8_t *str, uint32_t len, uint32_t timeout) {
	return uart3_recieve(str, len, timeout);
}

void sda_serial_transmit(uint8_t *str, uint32_t len) {
	uart3_transmit(str, len);
}

void sda_serial_enable() {
	sdaSerialEnabled = 1;
	HAL_UART3_MspInit(&huart3);
	MX_USART3_UART_Init();
}

void sda_serial_disable() {
	HAL_UART_AbortReceive(&huart3);
	MX_USART3_UART_DeInit();
	sdaSerialEnabled = 0;
}

uint8_t sda_serial_is_enabled() {
	return sdaSerialEnabled;
}

void sda_dbg_serial_enable() {
	HAL_UART_MspInit(&huart2);
	MX_USART2_UART_Init();
	sdaDbgSerialEnabled = 1;
}

void sda_dbg_serial_disable() {
	sdaDbgSerialEnabled = 0;
	MX_USART2_UART_DeInit();
}

uint8_t sda_dbg_serial_is_enabled() {
	return sdaDbgSerialEnabled;
}

uint32_t svp_random() {
	uint32_t ret = 0;
	HAL_RNG_GenerateRandomNumber(&rng, &ret);
	return ret/2;
}

void svp_set_lcd_state(lcdStateType state) {
	if(state == LCD_ON) {
		system_clock_set_normal();
		lcd_hw_wake();
		Lcd_on_flag = 200;
	} else if (state == LCD_OFF) {
		lcd_bl_off();
		Lcd_off_flag = 30;
	}
	svpSGlobal.lcdState = state;
}
void svp_set_backlight(uint8_t val) {
	lcd_hw_set_backlight(val);
}

void led_set_pattern(ledPatternType pat) {
	uint8_t x;
	if (pat == LED_ON) {
		for (x = 0; x < 10; x++) {
			led_pattern[x] = 1;
		}
	}

	if (pat == LED_OFF) {
		for (x = 0; x < 10; x++) {
			led_pattern[x] = 0;
		}
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

float get_batt_voltage() {
	// get current conversion constant
	if (boardRev == REV2B) {
		batt_adc_const = (VOLTAGE_REF_VAL_DEF) / (float)voltage_ref_val;
		//printf("measuring: ref: %u, const: %u, voltage:%u\n", voltage_ref_val, (uint32_t)(batt_adc_const*100000), (uint32_t) ((float)batt_val * batt_adc_const * 100.0));
		return ( (((float)batt_val) * batt_adc_const) / 0.6); //1.666 is a const of the battery voltage divider
	} else if (boardRev == REV1) {
		return (((float)batt_val) * batt_adc_const);
	}
}

uint8_t get_batt_percent() {
	uint8_t percent;
	percent = (uint8_t)((get_batt_voltage() - MIN_VOLTAGE) / ((MAX_VOLTAGE - MIN_VOLTAGE) / 100 ));
	if (percent > 100) {
		percent = 100;
	}
	return percent;
}

void system_clock_set_low(void) {

	// abychom si nekazili beeper
	// do not set low clock speed if beeper is on
	if(beep_flag != 0) {
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

	MX_USART2_UART_Init();
	lcd_bl_timer_OC_update();

	if (sda_serial_is_enabled()) {
		sda_serial_disable();
		sda_serial_enable();
	}

	cpuClkLowFlag = 1;
}

void system_clock_set_normal(void){
	// do not change clock speed if beeper is on
	// abychom si nekazili beeper
	if (beep_flag != 0) {
		return;
	}

	HAL_RCC_DeInit();
	SystemClock_Config();
	SystemCoreClockUpdate();
	MX_USART2_UART_Init();
	lcd_bl_timer_OC_update();

	if (sda_serial_is_enabled()) {
		sda_serial_disable();
		sda_serial_enable();
	}

	cpuClkLowFlag = 0;
}

void sda_sleep() {

	// set up the RTC alarm and pwr button wakeup

	tickLock = 0;
	touchSleep();
	if(cpuClkLowFlag == 0){
		system_clock_set_low();
	}
	rtc_set_1s_wkup();
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
	// enter sleep state
	HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
  sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);
  touchWake();
  tickLock = 1;
  //printf("sda leaving deep sleep\n");
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);

}
/*===========================================================================*/
void EXTI0_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	(void) (GPIO_Pin);
	if (svpSGlobal.powerMode == SDA_PWR_MODE_SLEEP && Lcd_off_flag == 0) {
		svpSGlobal.powerMode = SDA_PWR_MODE_NORMAL;
	}
}

void svs_hardErrHandler() {
	tickLock = 0;
	redraw_lock = 1;
	LCD_Fill(LCD_MixColor(255, 0, 0));
	LCD_DrawText_ext(32, 100, 0xFFFF, (uint8_t *)"Hard error occured!\nSDA-os will now reset!");
	Delay(50000000);
	HAL_NVIC_SystemReset();
	while(1);
}

void tick_update_buttons(uint8_t *btn) {
	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_A_PORT, SDA_BASE_BTN_A_PIN) == GPIO_PIN_SET){ // A
		btn[0] = 1;
	} else {
		btn[0] = 0;
	}
	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_LEFT_PORT, SDA_BASE_BTN_LEFT_PIN) == GPIO_PIN_SET){ // Left
		btn[1] = 1;
	} else {
		btn[1] = 0;
	}
	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_UP_PORT, SDA_BASE_BTN_UP_PIN) == GPIO_PIN_SET){ // Up
		btn[2] = 1;
	} else {
		btn[2] = 0;
	}
	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_DOWN_PORT, SDA_BASE_BTN_DOWN_PIN) == GPIO_PIN_SET){ // Down
		btn[3] = 1;
	} else {
		btn[3] = 0;
	}

	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_RIGHT_PORT, SDA_BASE_BTN_RIGHT_PIN) == GPIO_PIN_SET){ // Right
		btn[4] = 1;
	} else {
		btn[4] = 0;
	}
	if(HAL_GPIO_ReadPin(SDA_BASE_BTN_B_PORT, SDA_BASE_BTN_B_PIN) == GPIO_PIN_SET){ // B
		btn[5] = 1;
	} else {
		btn[5] = 0;
	}
}

void update_power_status() {
	if (boardRev == REV1) {
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_SET) {
			svpSGlobal.pwrType = POWER_USB;
		} else {
			svpSGlobal.pwrType = POWER_BATT;
		}
	} else {
		if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_7) == GPIO_PIN_SET) {
			svpSGlobal.pwrType = POWER_USB;
		} else {
			svpSGlobal.pwrType = POWER_BATT;
		}
	}
}

void sda_hw_sleep() {
	uptimeSleepStart = (uint32_t) svpSGlobal.timestamp - svpSGlobal.uptime;
	lcd_hw_sleep();
}

void updateTouchScreen(){
	redraw_lock = 1;
	static uint8_t touchPrev;
	static uint8_t touchNow;
	touchXY pRetval;

	svpSGlobal.touchValid = touch_get_xy(&pRetval);

	if (svpSGlobal.touchValid) {
		svpSGlobal.touchX = LCD_rotr_x(pRetval.x, pRetval.y);
		svpSGlobal.touchY = LCD_rotr_y(pRetval.x, pRetval.y);
	}

	if (svpSGlobal.touchValid) {
		touchNow = 1;
	} else {
		touchNow = 0;
	}
	if (svpSGlobal.touchType == EV_NONE) {
		if ((touchNow == 1) && (touchPrev == 0)) {
			svpSGlobal.touchType = EV_PRESSED;
		}

		if ((touchNow == 0) && (touchPrev == 1)) {
			svpSGlobal.touchType = EV_RELEASED;
		}
		if ((touchNow == 1) && (touchPrev == 1)) {
			svpSGlobal.touchType = EV_HOLD;
		}
		if ((touchNow == 0) && (touchPrev == 0)) {
			svpSGlobal.touchType = EV_NONE;
		}
		touchPrev = touchNow;
	}

	redraw_lock = 0;
}
/*============================The SysTick monster============================*/

void SysTick_Handler(void) {
	static uint16_t sec;

	static uint8_t led_state;
	static uint16_t led_cnt;
	static uint16_t batt_cnt;

	static uint8_t oldsec;

	static uint8_t btn[6];
	static uint8_t btn_old[6];

	static uint8_t powerOnLck;

	HAL_IncTick();
	svsLoadCounter++;

	static uint8_t pwrBtnPrev;

	if (Lcd_on_flag > 1){
		Lcd_on_flag--;
	}

	if (Lcd_on_flag == 1) {
		Lcd_on_flag = 0;
		lcd_bl_on();
		svp_set_backlight(svpSGlobal.lcdBacklight);
		if (Lcd_off_flag == 0) {
			setRedrawFlag();
		}
	}

	sda_base_spkr_irq_handler();

	if (tickLock == 1) {
		counter++;

		static uint32_t pwrLongPressCnt;
		// Power on with just press
		if((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) && (pwrBtnPrev == 0)) {
			if (svpSGlobal.lcdState == LCD_OFF) {
				svp_set_lcd_state(LCD_ON);
				system_clock_set_normal();
				powerOnLck = 1;
				svpSGlobal.powerMode = SDA_PWR_MODE_NORMAL;
				pwrLongPressCnt = 0;
			}
		}
		// pwr long press detection
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET && svpSGlobal.lcdState == LCD_ON) {
			pwrLongPressCnt++;
		}

		if (pwrLongPressCnt == 1500) {
			svpSGlobal.systemPwrLongPress = 1;
		}

		if((HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET) && (pwrBtnPrev == 1)) {
			if (pwrLongPressCnt > 1500) {
				pwrLongPressCnt = 0;
			} else {
				if (powerOnLck) {
					powerOnLck = 0;
				} else {
					// power off with release
					if (svpSGlobal.lcdState == LCD_ON) {
						svp_set_lcd_state(LCD_OFF);
					}
				}
			}
		}

		pwrBtnPrev = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

		tick_update_buttons(btn);
		uint8_t i;
		for (i = 0; i < 6; i++) {
			if (svpSGlobal.keyEv[i] == EV_NONE) {
				if ((btn[i] == 1) && (btn_old[i] == 0)) {
					svpSGlobal.keyEv[i] = EV_PRESSED;
					svpSGlobal.btnFlag = 1;
				}

				if ((btn[i] == 1) && (btn_old[i] == 1)) {
					svpSGlobal.keyEv[i] = EV_HOLD;
					svpSGlobal.btnFlag = 1;
				}

				if ((btn[i] == 0) && (btn_old[i] == 1)) {
					svpSGlobal.keyEv[i] = EV_RELEASED;
					svpSGlobal.btnFlag = 1;
				}

				btn_old[i] = btn[i];
			}
		}

		if (led_cnt < 100) {
			led_cnt++;
		} else {
			led_cnt = 0;
			//práce s patternem ledky
			if (led_pattern[led_counter] != led_state) {
				if (led_pattern[led_counter] == 1) {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
					led_state = 1;
				} else {
					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
					led_state = 0;
				}
			}
			if (led_counter < 9) {
				led_counter++;
			} else {
				led_counter = 0;
			}
		}

		if (irq_lock == 0) {
			irq_lock = 1;
			update_power_status();
			if(sec < 300) {
				sec++;
			} else {
				sec = 0;
				//event update času
				rtc_update_struct();

				sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);

				if(rtc.sec != oldsec) {
					if (Lcd_off_flag > 1) {
						Lcd_off_flag--;
					}
					if (Lcd_off_flag == 1) {
						Lcd_off_flag = 0;
						if(svpSGlobal.lcdState == LCD_OFF) {
							sda_hw_sleep();
						}
					}
				}
				oldsec = rtc.sec;

				if (batt_cnt < 59) {
					batt_array[batt_cnt] = getBatteryVoltage();
					batt_cnt++;
				} else {
					uint32_t temp;
					uint16_t i;

					temp = 0;
					for(i = 0; i < 59; i++) {
						temp += batt_array[i];
					}
					voltage_ref_val = getRefVoltage();
					batt_val = temp / 60 + temp % 60;

					temp = (uint32_t)(get_batt_voltage() * 10);
					svpSGlobal.battString[0] = ' ';
					svpSGlobal.battString[1] = temp / 10 + 48;
					svpSGlobal.battString[2] = '.';
					svpSGlobal.battString[3] = temp % 10 + 48;
					svpSGlobal.battString[4] = 'V';
					svpSGlobal.battString[5] = 0;

					svpSGlobal.battPercentage = get_batt_percent();
					batt_cnt = 0;
				}
			}

			if ((counter > 15)) { //překreslování a načtení nového stavu vstupů po cca 33ms
				counter = 0;

				if ((touch_lock == 0) && (svpSGlobal.lcdState == LCD_ON)) {
					updateTouchScreen();
				}

				if (!redraw_lock) {
					redraw_lock = 1;
					svp_irq(svpSGlobal);
					redraw_lock = 0;
				}
			}
			irq_lock = 0;
		}
		// counter for use inside SVS apps
		if (svsCounter > 0) {
			svsCounter--;
		}
	}
}

int main() {
	__initialize_hardware();

	boardRev = UNKNOWN;

	ADC_Measurement_const = BATT_ADC_CONST_DEF;

	batt_adc_const = BATT_ADC_CONST_DEF;

	tickLock = 0;
	redraw_lock = 1;

	sda_platform_gpio_init();
	sda_dbg_serial_enable();

	printf("SDA-WONDER\nStanda 2018\n\n");

	// drivers init
	rtc_init();
	rrand_init();
	beep_timer_init();
	backlight_timer_init();
	touchInit();

	//backlight init
	svpSGlobal.lcdBacklight = 255;
	svp_set_lcd_state(LCD_ON);
	svp_set_backlight(255);

	LCD_init(319, 479, OR_NORMAL);
	LCD_Fill(0xFFFF);
	sda_set_led(1);
	Delay(2000000);
	LCD_Fill(0x0);
	sda_set_led(0);
	Delay(2000000);
	LCD_Fill(0xFFFF);
	LCD_Fill(0x0);
	sda_set_led(1);
	LCD_setDrawArea(0,0,319,479);

	// UP on both board revisions goes straight to calibration
	if (HAL_GPIO_ReadPin(SDA_BASE_BTN_UP_PORT, SDA_BASE_BTN_UP_PIN) == GPIO_PIN_SET) {
		printf("LCD Calibbration!\n");
		sda_calibrate();
		sda_setLcdCalibrationFlag(1);
	}

	svp_mount();

	printf("ff init\n");

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);

	uint32_t count;

	if (ppm_get_width((uint8_t *) "splash.ppm") == 320) {
		draw_ppm(0, 0, 1,(uint8_t *) "splash.ppm");
		count = 25000000;
			for(; count != 0; count--) {
			if(HAL_GPIO_ReadPin(SDA_BASE_BTN_B_PORT, SDA_BASE_BTN_B_PIN) == GPIO_PIN_SET) {
				break;
			}
		}

	}else if (ppm_get_width((uint8_t *) "splash.ppm") == 160) {
		draw_ppm(0, 0, 2,(uint8_t *) "splash.ppm");
		count = 25000000;
		for(; count != 0; count--) {
			if(HAL_GPIO_ReadPin(SDA_BASE_BTN_B_PORT, SDA_BASE_BTN_B_PIN) == GPIO_PIN_SET){
				break;
			}
		}
	}

	// update time before jumping into main
	rtc_update_struct();
	sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);

	while(1) {
		sda_main_loop();

		if (svpSGlobal.powerMode == SDA_PWR_MODE_SLEEP && Lcd_off_flag == 0) {
			tickLock = 0;
			// SDA wil go to sleep for about a 1s, then RTC or PWRBUTTON wakes it up
			sda_sleep();
			// update time
			rtc_update_struct();
			sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);
			// update uptime
			svpSGlobal.uptime = (uint32_t)svpSGlobal.timestamp - uptimeSleepStart;
			tickLock = 1;
		}
	}
}
