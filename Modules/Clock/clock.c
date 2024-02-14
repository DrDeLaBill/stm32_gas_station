/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <stdbool.h>

#include "log.h"
#include "bmacro.h"
#include "hal_defs.h"


extern RTC_HandleTypeDef hrtc;


uint8_t clock_get_year()
{
    RTC_DateTypeDef date;
    if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return date.Year;
}

uint8_t clock_get_month()
{
    RTC_DateTypeDef date;
    if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return date.Month;
}

uint8_t clock_get_date()
{
    RTC_DateTypeDef date;
    if (HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return date.Date;
}

uint8_t clock_get_hour()
{
    RTC_TimeTypeDef time;
    if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return time.Hours;
}

uint8_t clock_get_minute()
{
    RTC_TimeTypeDef time;
    if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return time.Minutes;
}

uint8_t clock_get_second()
{
    RTC_TimeTypeDef time;
    if (HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 0;
    }
    return time.Seconds;
}

void clock_save_time(RTC_TimeTypeDef* time)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    if (time->Seconds > 59 || time->Hours > 23 || time->Minutes > 59) {
        return;
    } else {
        status = HAL_RTC_SetTime(&hrtc, time, RTC_FORMAT_BIN);
    }
    if (status != HAL_OK)
    {
		BEDUG_ASSERT(false, "Unable to set current time");
    }
}

void clock_save_date(RTC_DateTypeDef* date)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    if (date->Date > 31 || date->Month > 12) {
        return;
    } else {
        status = HAL_RTC_SetDate(&hrtc, date, RTC_FORMAT_BIN);
    }
    if (status != HAL_OK)
    {
		BEDUG_ASSERT(false, "Unable to set current date");
    }
}

bool clock_get_rtc_time(RTC_TimeTypeDef* time)
{
	return HAL_OK == HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
}

bool clock_get_rtc_date(RTC_DateTypeDef* date)
{
	return HAL_OK == HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
}

enum Months {
	JANUARY = 0,
	FEBRUARY,
	MARCH,
	APRIL,
	MAY,
	JUNE,
	JULY,
	AUGUST,
	SEPTEMBER,
	OCTOBER,
	NOVEMBER,
	DECEMBER
};

uint32_t clock_datetime_to_seconds(RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
{
	uint32_t days = date->Year * 365;
	if (date->Year > 0) {
		days += (date->Year / 4) + 1;
	}
	for (unsigned i = 0; i < (unsigned)(date->Month > 0 ? date->Month - 1 : 0); i++) {
		switch (i) {
		case JANUARY:
			days += 31;
			break;
		case FEBRUARY:
			days += ((date->Year % 4 == 0) ? 29 : 28);
			break;
		case MARCH:
			days += 31;
			break;
		case APRIL:
			days += 30;
			break;
		case MAY:
			days += 31;
			break;
		case JUNE:
			days += 30;
			break;
		case JULY:
			days += 31;
			break;
		case AUGUST:
			days += 31;
			break;
		case SEPTEMBER:
			days += 30;
			break;
		case OCTOBER:
			days += 31;
			break;
		case NOVEMBER:
			days += 30;
			break;
		case DECEMBER:
			days += 31;
			break;
		default:
			break;
		};
	}
	if (date->Date > 0) {
		days += (date->Date - 1);
	}
	uint32_t hours = days * 24 + time->Hours;
	uint32_t minutes = hours * 60 + time->Minutes;
	uint32_t seconds = minutes * 60 + time->Seconds;
	return seconds;
}

uint32_t clock_get_timestamp()
{
	RTC_DateTypeDef date = {0};
	RTC_TimeTypeDef time = {0};

	if (!clock_get_rtc_date(&date)) {
		BEDUG_ASSERT(false, "Unable to get current date");
		memset((void*)&date, 0, sizeof(date));
	}

	if (!clock_get_rtc_time(&time)) {
		BEDUG_ASSERT(false, "Unable to get current time");
		memset((void*)&time, 0, sizeof(time));
	}

	return clock_datetime_to_seconds(&date, &time);
}
