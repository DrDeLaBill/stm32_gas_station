/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <stdbool.h>

#include "log.h"
#include "bmacro.h"
#include "hal_defs.h"


extern RTC_HandleTypeDef hrtc;


typedef enum _Months {
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
} Months;


uint8_t _get_days_in_month(uint8_t year, Months month);


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
    if (time->Seconds >= SECONDS_PER_MINUTE ||
		time->Minutes >= MINUTES_PER_HOUR ||
		time->Hours   >= HOURS_PER_DAY
	) {
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
    if (date->Date > DAYS_PER_MONTH_MAX || date->Month > MONTHS_PER_YEAR) {
        return;
    } else {
    	/* calculating weekday begin */
    	RTC_TimeTypeDef time = {0};
    	uint32_t seconds = clock_datetime_to_seconds(date, &time);
    	clock_seconds_to_datetime(seconds, date, &time);
    	/* calculating weekday end */
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

uint32_t clock_datetime_to_seconds(RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
{
	uint32_t days = date->Year * DAYS_PER_YEAR;
	if (date->Year > 0) {
		days += (uint32_t)((date->Year - 1) / LEAP_YEAR_PERIOD) + 1;
	}
	for (unsigned i = 0; i < (unsigned)(date->Month > 0 ? date->Month - 1 : 0); i++) {
		days += _get_days_in_month(date->Year, i);
	}
	days += date->Date;
	days -= 1;
	uint32_t hours = days * HOURS_PER_DAY + time->Hours;
	uint32_t minutes = hours * MINUTES_PER_HOUR + time->Minutes;
	uint32_t seconds = minutes * SECONDS_PER_MINUTE + time->Seconds;
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

void clock_seconds_to_datetime(uint32_t seconds, RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
{
	memset(date, 0, sizeof(RTC_DateTypeDef));
	memset(time, 0, sizeof(RTC_TimeTypeDef));

	time->Seconds = (uint8_t)(seconds % SECONDS_PER_MINUTE);
	uint32_t minutes = seconds / SECONDS_PER_MINUTE;

	time->Minutes = (uint8_t)(minutes % MINUTES_PER_HOUR);
	uint32_t hours = minutes / MINUTES_PER_HOUR;

	time->Hours = (uint8_t)(hours % HOURS_PER_DAY);
	uint32_t days = 1 + hours / HOURS_PER_DAY;

	date->WeekDay = (RTC_WEEKDAY_THURSDAY + days) % (DAYS_PER_WEEK);
	if (!date->WeekDay) {
		date->WeekDay = RTC_WEEKDAY_MONDAY;
	}
	date->Month = 1;
	while (days) {
		uint16_t days_in_year = (date->Year % LEAP_YEAR_PERIOD > 0) ? DAYS_PER_YEAR : DAYS_PER_LEAP_YEAR;
		if (days > days_in_year) {
			days -= days_in_year;
			date->Year++;
			continue;
		}

		uint8_t days_in_month = _get_days_in_month(date->Year, date->Month - 1);
		if (days > days_in_month) {
			days -= days_in_month;
			date->Month++;
			continue;
		}

		date->Date = (uint8_t)days;
		break;
	}
}

uint8_t _get_days_in_month(uint8_t year, Months month)
{
	switch (month) {
	case JANUARY:
		return 31;
	case FEBRUARY:
		return ((year % 4 == 0) ? 29 : 28);
	case MARCH:
		return 31;
	case APRIL:
		return 30;
	case MAY:
		return 31;
	case JUNE:
		return 30;
	case JULY:
		return 31;
	case AUGUST:
		return 31;
	case SEPTEMBER:
		return 30;
	case OCTOBER:
		return 31;
	case NOVEMBER:
		return 30;
	case DECEMBER:
		return 31;
	default:
		break;
	};
	return 0;
}
