// ----------------------------------------------------------------------------
#include "sda_platform.h"
#include "math.h"

/*=========================== Globals =======================================*/
// HAL handles
ADC_HandleTypeDef g_AdcHandle;
ADC_HandleTypeDef g_AdcHandle2;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

// SDA Globals
FATFS FatFs;
svpStatusStruct svpSGlobal;
volatile wonderBoardRevisions boardRev;

uint32_t uptimeSleepStart;

// misc system locks
volatile sdaLockState touch_lock;  // disables touch in irq
volatile sdaLockState irq_lock;    // disables touch, redraw in irq and battery measurement
volatile sdaLockState tick_lock;   // disables all systick stuff

// systick globals
volatile uint32_t counter;
volatile uint16_t svsCounter;
volatile uint16_t svsLoadCounter;

volatile uint8_t serialBuf[16];
volatile uint8_t Lcd_on_flag;
volatile uint8_t Lcd_off_flag;

volatile uint32_t backlight_scaler;

// led pattern array
uint8_t led_pattern[10];
uint16_t led_counter;

volatile uint8_t sdaWakeupFlag;

volatile uint8_t cpuClkLowFlag; // for the current state of the CPU speed

volatile float ADC_Measurement_const;

// battery measurement
extern volatile uint32_t batt_val;
extern volatile float batt_adc_const;

void __initialize_hardware(void);
/*=========================== Local headers =================================*/
void Delay(__IO uint32_t nCount);
void beep_timer_init();
void rrand_init();


void Delay(__IO uint32_t nCount) {
  for(; nCount != 0; nCount--);
}


void sda_sleep() {
	tick_lock = SDA_LOCK_LOCKED;
	touchSleep();
	if(cpuClkLowFlag == 0){
		system_clock_set_low();
	}

	if (svpSGlobal.powerSleepMode == SDA_PWR_MODE_SLEEP_LOW) {
		rtc_set_wkup(128);
	} else if (svpSGlobal.powerSleepMode == SDA_PWR_MODE_SLEEP_NORMAL) {
		rtc_set_wkup(1000);
	} else 	if (svpSGlobal.powerSleepMode == SDA_PWR_MODE_SLEEP_DEEP) {
		rtc_set_wkup(8000);
	}

	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_RESET);
	// enter sleep state
	HAL_SuspendTick();
  HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  HAL_ResumeTick();
  sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);
  touchWake();
  tick_lock = SDA_LOCK_UNLOCKED;
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_7, GPIO_PIN_SET);

}


/*===========================================================================*/
void EXTI0_IRQHandler(void) {
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	(void) (GPIO_Pin);
	if (svpSGlobal.powerMode == SDA_PWR_MODE_SLEEP && Lcd_off_flag == 0) {
		sdaWakeupFlag = 1;
	}
}


void lowBattCheckAndHalt() {
	if (((uint32_t)(get_batt_voltage() * 10) < 31) && (svpSGlobal.pwrType == POWER_BATT) && (batt_val != 0)) {
		while(1) {
			tick_lock = SDA_LOCK_LOCKED;
			LCD_Fill(LCD_MixColor(255, 0, 0));
			LCD_DrawText_ext(32, 100, 0xFFFF, (uint8_t *)"Low battery!");
			Delay(90000000);
			svp_set_lcd_state(LCD_OFF);
			sda_set_led(0);
			lcd_hw_sleep();
			HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
			update_power_status();
			if (svpSGlobal.pwrType == POWER_USB) {
				HAL_NVIC_SystemReset();
				while(1);
			}
		}
	}
}


/*=========================== The SysTick Monster ===========================*/

void SysTick_Handler(void) {
	static uint16_t sec;

	static uint8_t led_state;
	static uint16_t led_cnt;

	static uint8_t oldsec;

	static uint8_t btn[6];
	static uint8_t btn_old[6];

	static uint8_t powerOnLck;

	HAL_IncTick();
	svsLoadCounter++;
	svpSGlobal.uptimeMs++;

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

	if (tick_lock == SDA_LOCK_UNLOCKED) {
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
		sdaWakeupFlag = 0; // button was handled
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

		if (irq_lock == SDA_LOCK_UNLOCKED) {
			irq_lock = SDA_LOCK_LOCKED;
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
				measureBatteryVoltage();
			}

			if ((counter > 15)) { // Touch update and redraw of the system bar
				counter = 0;

				if ((touch_lock == SDA_LOCK_UNLOCKED) && (svpSGlobal.lcdState == LCD_ON)) {
					updateTouchScreen();
				}

				svp_irq(svpSGlobal);
			}
			irq_lock = SDA_LOCK_UNLOCKED;
		}

		// counter for use inside SVS apps
		if (svsCounter > 0) {
			svsCounter--;
		}
	}
}


int main() {
	__initialize_hardware();

	// Set up the consts
	boardRev = UNKNOWN;
	ADC_Measurement_const = BATT_ADC_CONST_DEF;
	batt_adc_const = BATT_ADC_CONST_DEF;

	tick_lock = SDA_LOCK_LOCKED;

	irq_lock = SDA_LOCK_UNLOCKED;

	sda_platform_gpio_init();
	sda_dbg_serial_enable();

	printf("SDA-WONDER\nStanda 2019\n\n");

	// Drivers init
	rtc_init();
	rrand_init();
	beep_timer_init();
	backlight_timer_init();
	touchInit();

	// Backlight initial setup
	svpSGlobal.lcdBacklight = 255;
	svp_set_lcd_state(LCD_ON);
	svp_set_backlight(255);

	LCD_init(319, 479, OR_NORMAL);

	// power status update
	update_power_status();

	// if not powered from usb:
	if (svpSGlobal.pwrType == POWER_BATT) {
		batt_val = 0;
		//measure the initial battery state
		while (batt_val == 0) {
			measureBatteryVoltage();
		}
		// and eventualy halt
		lowBattCheckAndHalt();
	}

	// Blink with LCD and with notif. led
	lcd_bw_test();

	// UP on both board revisions goes straight to calibration
	if (HAL_GPIO_ReadPin(SDA_BASE_BTN_UP_PORT, SDA_BASE_BTN_UP_PIN) == GPIO_PIN_SET) {
		printf("LCD Calibration!\n");
		sda_calibrate();
		sda_setLcdCalibrationFlag(1);
	}

	// FS mount is performed after the power check, to prevent SD corruption
	svp_mount();

	sda_set_led(0);

	show_splash();

	// Update time before jumping into main
	rtc_update_struct();
	sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);

	while(1) {
		sda_main_loop();

		// Sleep mode handling
		if (svpSGlobal.powerMode == SDA_PWR_MODE_SLEEP && Lcd_off_flag == 0 && sdaWakeupFlag == 0) {
			tick_lock = SDA_LOCK_LOCKED;
			sda_sleep();
			// update time
			rtc_update_struct();
			sda_irq_update_timestruct(rtc.year, rtc.month, rtc.day, rtc.weekday, rtc.hour, rtc.min, rtc.sec);
			// update uptime
			svpSGlobal.uptime = (uint32_t)svpSGlobal.timestamp - uptimeSleepStart;
			// update battery state
			if (svpSGlobal.powerSleepMode != SDA_PWR_MODE_SLEEP_LOW) {
			  tick_lock = SDA_LOCK_LOCKED;
			  measureBatteryVoltage();
			}
			tick_lock = SDA_LOCK_UNLOCKED;
		}

		// Also halt if battery voltage is too low
		lowBattCheckAndHalt();
	}
}
