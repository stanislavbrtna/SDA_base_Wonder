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
volatile uint16_t sdaAppCounter;
volatile uint16_t svsLoadCounter;

volatile uint8_t serialBuf[16];
volatile uint8_t Lcd_on_flag;
volatile uint8_t Lcd_off_flag;

volatile uint32_t backlight_scaler;

volatile uint8_t sdaWakeupFlag;

// battery measurement
extern volatile uint32_t batt_val;
extern volatile float batt_adc_const;
volatile float ADC_Measurement_const;

void __initialize_hardware(void);

void Delay(__IO uint32_t nCount);
void beep_timer_init();
void rrand_init();

void Delay(__IO uint32_t nCount) {
  for(; nCount != 0; nCount--);
}


/*=========================== The SysTick Monster ===========================*/

void SysTick_Handler(void) {
	static uint16_t sec;
	static uint8_t oldsec;

	static uint8_t powerOnLck;
  static uint8_t pwrBtnPrev;

	HAL_IncTick();
	svsLoadCounter++;
	svpSGlobal.uptimeMs++;

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

		tick_update_buttons();

		tick_update_status_led();

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
		if (sdaAppCounter > 0) {
			sdaAppCounter--;
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
	sda_usb_serial_enable();

	printf("SDA-WONDER\nStanda 2019-2023\n\n");

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

	if (sda_card_inserted() == 0) {
	    LCD_Fill(LCD_MixColor(255, 0, 0));
	    LCD_DrawText_ext(32, 100, 0xFFFF, (uint8_t *)"SDA Error:\nSD card not found!\nPlease insert SD card.");

	    LCD_DrawText_ext(32, 320, 0xFFFF, (uint8_t *)"SDA-OS v."SDA_OS_VERSION);
	#ifdef PC
	    getchar();
	#else
	    while(1){
	      if (sda_card_inserted()) {
	        break;
	      }
	    }
	#endif
	}

	// FS mount is performed after the power check, to prevent SD corruption
	svp_mount();

	sda_set_led(0);

	show_splash();

	touch_lock = SDA_LOCK_UNLOCKED;

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
			svpSGlobal.uptimeMs = svpSGlobal.uptime * 1000;
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
