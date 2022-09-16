/*
Copyright (c) 2018 Stanislav Brtna

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "touch.h"

//#define TOUCH_DBG

touchCalibDataStruct touchCalibData;

extern ADC_HandleTypeDef g_AdcHandle;
extern ADC_HandleTypeDef g_AdcHandle2;


static void touch_delay(__IO uint32_t nCount);
static void touchAdcInit(void);

static void touch_delay(__IO uint32_t nCount) {
  for(; nCount != 0; nCount--);
}

uint8_t calibrationFlag;

void sda_setLcdCalibrationFlag(uint8_t val) {
	calibrationFlag = val;
}

uint8_t svp_getLcdCalibrationFlag() {
	return calibrationFlag;
}

static void touchAdcInit(void) {
	__GPIOB_CLK_ENABLE();
	__GPIOA_CLK_ENABLE();
	__ADC1_CLK_ENABLE();
	g_AdcHandle.Instance = ADC1;

	g_AdcHandle.Init.ClockPrescaler        = ADC_CLOCKPRESCALER_PCLK_DIV2;
	g_AdcHandle.Init.Resolution            = ADC_RESOLUTION_12B;
	g_AdcHandle.Init.ScanConvMode          = ENABLE;
	g_AdcHandle.Init.ContinuousConvMode    = ENABLE;
	g_AdcHandle.Init.DiscontinuousConvMode = DISABLE;
	g_AdcHandle.Init.NbrOfDiscConversion   = 0;
	g_AdcHandle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
	g_AdcHandle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
	g_AdcHandle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
	g_AdcHandle.Init.NbrOfConversion       = 1;
	g_AdcHandle.Init.DMAContinuousRequests = ENABLE;
	g_AdcHandle.Init.EOCSelection          = DISABLE;

	if(HAL_ADC_Init(&g_AdcHandle) != HAL_OK) {
		printf("touch_dbg: hal init FAIL\n");
	}

	ADC_ChannelConfTypeDef adcChannel;

	adcChannel.Channel = ADC_CHANNEL_6;
	adcChannel.Rank    = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;

	if (HAL_ADC_ConfigChannel(&g_AdcHandle, &adcChannel) != HAL_OK) {
		printf("touch_dbg: hal c6 channel init FAIL\n");
	}


	__ADC2_CLK_ENABLE();
	g_AdcHandle2.Instance = ADC2;

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

	adcChannel.Channel = ADC_CHANNEL_7;
	adcChannel.Rank    = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;

	if (HAL_ADC_ConfigChannel(&g_AdcHandle2, &adcChannel) != HAL_OK) {
		printf("touch_dbg: hal channel 7 init FAIL\n");
	}

	if(HAL_ADC_Init(&g_AdcHandle2)!= HAL_OK ) {
		printf("touch_dbg: hal init FAIL\n");
	}
}

void touchAdcInitExt(void) {
	touchAdcInit();
}

#ifdef TOUCH_USE_BATTERY_MEASUREMENT
uint16_t getBatteryVoltage() {
	ADC_ChannelConfTypeDef adcChannel;
	GPIO_InitTypeDef gpioInit;
	__GPIOB_CLK_ENABLE();
	__GPIOA_CLK_ENABLE();
	__ADC1_CLK_ENABLE();
	__ADC2_CLK_ENABLE();

	// Deinit the ADCs, init battery measurement setup,
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
	adcChannel.Channel = ADC_CHANNEL_8;
	adcChannel.Rank    = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;

	if(HAL_ADC_Init(&g_AdcHandle2) != HAL_OK) {
		printf("batt_dbg: hal init FAIL\n");
	}

	if (HAL_ADC_ConfigChannel(&g_AdcHandle2, &adcChannel) != HAL_OK) {
		printf("batt_dbg: hal channel 7 init FAIL\n");
	}

	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_7);

	//PB0 ain - battery
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_0);
	gpioInit.Pin   = GPIO_PIN_0;
	gpioInit.Mode  = GPIO_MODE_ANALOG;
	gpioInit.Pull  = GPIO_NOPULL;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &gpioInit);

	HAL_ADC_Start(&g_AdcHandle2);
	if (HAL_ADC_PollForConversion(&g_AdcHandle2, 1000000) == HAL_OK) {
		measuredVoltage = HAL_ADC_GetValue(&g_AdcHandle2);
	}
	HAL_ADC_Stop(&g_AdcHandle2);

	g_AdcHandle.Instance = ADC1;

	HAL_ADC_DeInit(&g_AdcHandle);
	HAL_ADC_DeInit(&g_AdcHandle2);

	touchAdcInit();

	return measuredVoltage;
}

uint16_t getRefVoltage() {
	ADC_ChannelConfTypeDef adcChannel;
	GPIO_InitTypeDef gpioInit;
	__GPIOB_CLK_ENABLE();
	__GPIOA_CLK_ENABLE();
	__ADC1_CLK_ENABLE();
	__ADC2_CLK_ENABLE();

	// Deinit the ADCs, init battery measurement setup,
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
	adcChannel.Channel = ADC_CHANNEL_9;
	adcChannel.Rank    = 1;
	adcChannel.SamplingTime = ADC_SAMPLETIME_480CYCLES;
	adcChannel.Offset = 0;

	if(HAL_ADC_Init(&g_AdcHandle2) != HAL_OK) {
		printf("batt_dbg: hal init FAIL\n");
	}

	if (HAL_ADC_ConfigChannel(&g_AdcHandle2, &adcChannel) != HAL_OK) {
		printf("batt_dbg: hal channel 7 init FAIL\n");
	}

	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_6);
	HAL_GPIO_DeInit(GPIOA, GPIO_PIN_7);

	//PB0 ain - battery
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);
	gpioInit.Pin   = GPIO_PIN_1;
	gpioInit.Mode  = GPIO_MODE_ANALOG;
	gpioInit.Pull  = GPIO_NOPULL;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOB, &gpioInit);

	HAL_ADC_Start(&g_AdcHandle2);
	if (HAL_ADC_PollForConversion(&g_AdcHandle2, 1000000) == HAL_OK) {
		measuredVoltage = HAL_ADC_GetValue(&g_AdcHandle2);
	}
	HAL_ADC_Stop(&g_AdcHandle2);

	g_AdcHandle.Instance = ADC1;

	HAL_ADC_DeInit(&g_AdcHandle);
	HAL_ADC_DeInit(&g_AdcHandle2);

	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_1);
	touchAdcInit();

	return measuredVoltage;
}
#endif

void touchInit() {
	touchAdcInit();

	// todo: add sane calibration defaults to the
	// touchCalibData struct
}

void touchSleep() {
	__ADC1_CLK_DISABLE();
		__ADC2_CLK_DISABLE();
}
void touchWake() {
	__ADC1_CLK_ENABLE();
	__ADC2_CLK_ENABLE();
}
static void set_ain(GPIO_TypeDef *port, uint32_t pin){
	GPIO_InitTypeDef gpioInit;
	HAL_GPIO_DeInit(port, pin);
	gpioInit.Pin   = pin;
	gpioInit.Mode  = GPIO_MODE_ANALOG;
	gpioInit.Pull  = GPIO_NOPULL;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(port, &gpioInit);
}

static void set_gnd(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef gpioInit;
	HAL_GPIO_DeInit(port, pin);
	gpioInit.Pin   = pin;
	gpioInit.Mode  = GPIO_MODE_OUTPUT_PP;;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	gpioInit.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(port, &gpioInit);
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static void set_in(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef gpioInit;
	HAL_GPIO_DeInit(port, pin);
	gpioInit.Pin   = pin;
	gpioInit.Mode  = GPIO_MODE_INPUT;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	gpioInit.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(port, &gpioInit);
}

static void set_out(GPIO_TypeDef *port, uint32_t pin) {
	GPIO_InitTypeDef gpioInit;
	HAL_GPIO_DeInit(port, pin);
	gpioInit.Pin   = pin;
	gpioInit.Mode  = GPIO_MODE_OUTPUT_PP;;
	gpioInit.Speed = GPIO_SPEED_HIGH;
	gpioInit.Pull  = GPIO_NOPULL;
	HAL_GPIO_Init(port, &gpioInit);
	HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

uint8_t touch_read_xy(touchXY *result) {
	volatile uint32_t x = 0;
	volatile uint32_t y = 0;
	uint8_t valid = 0;

	set_gnd(GPIOA, GPIO_PIN_7); // ADC 2
	set_ain(GPIOA, GPIO_PIN_6); // ADC 1
	set_out(GPIOB, GPIO_PIN_8); // P2
	set_in(GPIOB, GPIO_PIN_9); // P1

	HAL_ADC_Start(&g_AdcHandle);

	if (HAL_ADC_PollForConversion(&g_AdcHandle, 1000000) == HAL_OK) {
		y = HAL_ADC_GetValue(&g_AdcHandle);
	}
	HAL_ADC_Stop(&g_AdcHandle);

	set_ain(GPIOA, GPIO_PIN_7); // ADC 2
	set_gnd(GPIOA, GPIO_PIN_6); // ADC 1
	set_in(GPIOB, GPIO_PIN_8); // P2
	set_out(GPIOB, GPIO_PIN_9); // P1


	HAL_ADC_Start(&g_AdcHandle2);

	if (HAL_ADC_PollForConversion(&g_AdcHandle2, 1000000) == HAL_OK){
		x = HAL_ADC_GetValue(&g_AdcHandle2);
	}
	HAL_ADC_Stop(&g_AdcHandle2);

	valid = 1; // result is at this point always valid

	result->x = x;
	result->y = y;

	//printf("measured: x:%u y:%u\n", x, y);

	return valid;
}

// Returns median of 30 subsequent touch screen readouts
uint8_t touch_read_xy_m30(touchXY *result) {
	touchXY prac;
	uint16_t x[31];
	uint16_t y[31];
	uint16_t i, j, temp;

	// median computation code was taken from from
	// https://en.wikiversity.org/wiki/C_Source_Code/Find_the_median_and_mean

	for (i = 0; i < 30; i++) {
		x[i] = 0;
		y[i] = 0;
	}

	for (i = 0; i < 30; i++) {
		prac.x = 0;
		prac.y = 0;
		if (touch_read_xy(&prac)) {
			x[i] = prac.x;
			y[i] = prac.y;
		} else {
			return 0;
		}
	}

	for (i = 0; i < 30 - 1; i++) {
		for (j = i + 1; j < 30; j++) {
			if (x[j] < x[i]) {
				// swap elements
				temp = x[i];
				x[i] = x[j];
				x[j] = temp;
			}
		}
	}

	for(i = 0; i < 30 - 1; i++) {
		for(j = i + 1; j < 30; j++) {
			if(y[j] < y[i]) {
				// swap elements
				temp = y[i];
				y[i] = y[j];
				y[j] = temp;
			}
		}
	}

	result->x = x[15];
	result->y = y[15];

	// validation of values
	if ((result->x < 3700)
			&& (result->y < 4000)
			&& (result->x > 350)
			&& (result->y > 350)) {
		// ok
		return 1;
	} else {
		// read failed
		return 0;
	}
}

uint8_t touch_get_xy(touchXY *result) {
	touchXY res;
	uint8_t i = 0;

	// read touch
	if (touch_read_xy(&res)) {
		touch_delay(300000);
		// wait for it to stabilize
		if (touch_read_xy_m30(&res)) {

			// get the values
		  if ((res.x < touchCalibData.cx) && (res.y < touchCalibData.cy)) {
		    i = 0;
		  }

		  if ((res.x > touchCalibData.cx) && (res.y < touchCalibData.cy)) {
		    i = 1;
		  }

		  if ((res.x < touchCalibData.cx) && (res.y > touchCalibData.cy)) {
        i = 2;
      }

      if ((res.x > touchCalibData.cx) && (res.y > touchCalibData.cy)) {
        i = 3;
      }

			result->x = touchCalibData.a[i] * res.x + touchCalibData.b[i];
			result->y = touchCalibData.c[i] * res.y + touchCalibData.d[i];


			// fix limits
			if (result->y == 0) {
				result->y = 1;
			}

			if ((result->x) > 320) {
				result->x = 318;
			}

			if (result->y > 480) {
				result->y = 478;
			}

#ifdef TOUCH_DBG
			printf("Touch: x:%u y:%u out: x:%u y:%u \n",
					res.x,
					res.y,
					result->x,
					result->y
			);
			LCD_set_XY(result->x, result->y, result->x, result->y);
			LCD_draw_point_wrp(0xFFFF);
#endif
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

void svp_set_calibration_data(touchCalibDataStruct input) {
  touchCalibData.cx = input.cx;
  touchCalibData.cy = input.cy;
	for (uint8_t i = 0; i < 4; i++) {
	  touchCalibData.a[i] = input.a[i];
	  touchCalibData.b[i] = input.b[i];
	  touchCalibData.c[i] = input.c[i];
	  touchCalibData.d[i] = input.d[i];
	}
}

// draw the calibration target
void touch_draw_cpoint(uint16_t x, uint16_t y, uint16_t color) {
	LCD_DrawLine(x, y - 5, x, y + 5, color);
	LCD_DrawLine(x-  5, y, x + 5, y, color);
	LCD_DrawRectangle(x - 1, y - 1, x + 1, y + 1, color);
}


/*
 * calibration points
 * 0:64x64   1:256x64
 *
 *    2:160x240
 *
 * 3:64x416  4:256x416
 * */

// draw all the calibration targets
void redraw_cpoints(uint8_t active, uint16_t * x_pos, uint16_t * y_pos) {
  LCD_setDrawArea(0, 0, 319, 479);
  LCD_Fill(0x0000);

  for(uint8_t i = 0; i < 5; i++){
    if (i == active) {
      touch_draw_cpoint(x_pos[i], y_pos[i], LCD_MixColor(255, 0, 0));
    } else {
      touch_draw_cpoint(x_pos[i], y_pos[i], 0xFFFF);
    }
  }
}

// Calibration routine
void sda_calibrate () {
	touchXY res;
	float rx[5];
	float ry[5];
	uint16_t i = 0;
	touchCalibDataStruct t;

	uint16_t x_pos[5] = {64, 256, 160, 64, 256};
	uint16_t y_pos[5] = {64, 64, 240, 416, 416};

	while (1) {

	  // get data
	  for (i = 0; i < 5; i++) {
	    redraw_cpoints(i, x_pos, y_pos);
	    while (touch_read_xy_m30(&res) == 0);
      touch_delay(300000);
      touch_read_xy_m30(&res);
      rx[i] = (float) res.x;
      ry[i] = (float) res.y;

      if (i == 2) {
        t.cx = res.x;
        t.cy = res.y;
      }

      while (touch_read_xy_m30(&res) == 1);
      touch_delay(300000);
	  }

	  // compute
	  t.c[0] = (y_pos[2] - y_pos[0]) / (ry[2] - ry[0]);
	  t.a[0] = (x_pos[2] - x_pos[0]) / (rx[2] - rx[0]);
	  t.b[0] = x_pos[0] - t.a[0] * rx[0];
	  t.d[0] = y_pos[2] - t.c[0] * ry[2];


	  t.c[1] = (y_pos[2] - y_pos[1]) / (ry[2] - ry[1]);
    t.a[1] = (x_pos[1] - x_pos[2]) / (rx[1] - rx[2]);
    t.b[1] = x_pos[1] - t.a[1] * rx[1];
    t.d[1] = y_pos[2] - t.c[1] * ry[2];

    t.c[2] = (y_pos[3] - y_pos[2]) / (ry[3] - ry[2]);
    t.a[2] = (x_pos[2] - x_pos[3]) / (rx[2] - rx[3]);
    t.b[2] = x_pos[3] - t.a[2] * rx[3];
    t.d[2] = y_pos[2] - t.c[2] * ry[2];

    t.c[3] = (y_pos[4] - y_pos[2]) / (ry[4] - ry[2]);
    t.a[3] = (x_pos[4] - x_pos[2]) / (rx[4] - rx[2]);
    t.b[3] = x_pos[4] - t.a[3] * rx[4];
    t.d[3] = y_pos[2] - t.c[3] * ry[2];

	  // set the new calibration data
	  svp_set_calibration_data(t);

		LCD_Fill(0x0000);
#ifndef LANG_VAL
		LCD_DrawText_ext(80, 240, 0xFFFF, (uint8_t *)"Test Kalibrace");
		LCD_DrawText_ext(10, 420, 0xFFFF, (uint8_t *)"Rekalibrovat");
		LCD_DrawText_ext(280, 420, 0xFFFF, (uint8_t *)"Ok");
#else
#if LANG_VAL==0
		LCD_DrawText_ext(80, 240, 0xFFFF, (uint8_t *)"Test Kalibrace");
		LCD_DrawText_ext(40, 260, 0xFFFF, (uint8_t *)"(Použijte tlačítka k potvrzení)");
		LCD_DrawText_ext(10, 420, 0xFFFF, (uint8_t *)"Rekalibrovat");
		LCD_DrawText_ext(280, 420, 0xFFFF, (uint8_t *)"Ok");
#endif

#if LANG_VAL==1
		LCD_DrawText_ext(80, 240, 0xFFFF, (uint8_t *)"Calibration test");
		LCD_DrawText_ext(40, 260, 0xFFFF, (uint8_t *)"(use buttons to select option)");
		LCD_DrawText_ext(10, 420, 0xFFFF, (uint8_t *)"Calibrate again");
		LCD_DrawText_ext(280, 420, 0xFFFF, (uint8_t *)"Ok");
#endif

#endif
		// test it
		while (1) {
			if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_0) == GPIO_PIN_SET) {
				break;
			}
			if (HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_SET) {
				return;
			}
			// by showing a point where we touch
			if(touch_get_xy(&res)) {
				LCD_DrawPoint(res.x, res.y, 0xFFF);
			}
		}
	}
}

