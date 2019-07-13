/*
Copyright (c) 2016 Stanislav Brtna

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

#ifndef RTC_RTC_H_
#define RTC_RTC_H_

#include "../sda_platform.h"

typedef struct {
	volatile uint8_t  sec;
	volatile uint8_t  min;
	volatile uint8_t  hour;
	volatile uint8_t  day;
	volatile uint8_t  weekday;
	volatile uint8_t  month;
	volatile uint16_t year;
} rtcTimeDate;

extern rtcTimeDate rtc;

void rtc_init();
void rtc_update_struct();
void rtc_set_wkup(uint32_t ms);
void rtc_set_time(
									uint16_t year,
									uint8_t  day,
									uint8_t  wkday,
									uint8_t  month,
									uint8_t  hour,
									uint8_t  min,
									uint8_t  sec
									);

#endif
