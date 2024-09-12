/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <stdbool.h>

#include "glog.h"
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


static const char TAG[] = "CLCK";


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

bool clock_save_time(const RTC_TimeTypeDef* time)
{
    HAL_StatusTypeDef status = HAL_ERROR;
    if (time->Seconds >= SECONDS_PER_MINUTE ||
		time->Minutes >= MINUTES_PER_HOUR ||
		time->Hours   >= HOURS_PER_DAY
	) {
        return false;
    }
    RTC_TimeTypeDef tmpTime = {0};
    memcpy((void*)&tmpTime, (void*)time, sizeof(tmpTime));

	HAL_PWR_EnableBkUpAccess();
	status = HAL_RTC_SetTime(&hrtc, &tmpTime, RTC_FORMAT_BIN);
	HAL_PWR_DisableBkUpAccess();

	BEDUG_ASSERT(status == HAL_OK, "Unable to set current time");
	if (status != HAL_OK) {
		printTagLog(
			TAG,
			"clock_save_time: time=%02u:%02u:%02u",
			tmpTime.Hours,
			tmpTime.Minutes,
			tmpTime.Seconds
		);
	}
    return status == HAL_OK;
}

bool clock_save_date(const RTC_DateTypeDef* date)
{
	RTC_TimeTypeDef tmpTime = {0};
	RTC_DateTypeDef tmpDate = {0};
    uint32_t seconds = 0;
    HAL_StatusTypeDef status = HAL_ERROR;
    if (date->Date > DAYS_PER_MONTH_MAX || date->Month > MONTHS_PER_YEAR) {
        return false;
    } else {
    	/* calculating weekday begin */
    	memcpy((void*)&tmpDate, (void*)date, sizeof(tmpDate));
    	seconds = clock_datetime_to_seconds(&tmpDate, &tmpTime);
    	clock_seconds_to_datetime(seconds, &tmpDate, &tmpTime);
        if (!tmpDate.WeekDay) {
        	BEDUG_ASSERT(false, "Error calculating clock weekday");
        	tmpDate.WeekDay = RTC_WEEKDAY_MONDAY;
        }
    	/* calculating weekday end */

    	HAL_PWR_EnableBkUpAccess();
        status = HAL_RTC_SetDate(&hrtc, &tmpDate, RTC_FORMAT_BIN);
    	HAL_PWR_DisableBkUpAccess();
    }
	BEDUG_ASSERT(status == HAL_OK, "Unable to set current date");
	if (status != HAL_OK) {
		printTagLog(
			TAG,
			"clock_save_date: seconds=%lu, time=20%02u-%02u-%02u weekday=%u",
			seconds,
			tmpDate.Year,
			tmpDate.Month,
			tmpDate.Date,
			tmpDate.WeekDay
		);
	}
    return status == HAL_OK;
}

bool clock_save_seconds(const uint32_t seconds)
{
	RTC_DateTypeDef date = {0};
	RTC_TimeTypeDef time = {0};
	clock_seconds_to_datetime(seconds, &date, &time);

	if (!clock_save_date(&date)) {
		return false;
	}

	return clock_save_time(&time);
}

bool clock_get_rtc_time(RTC_TimeTypeDef* time)
{
	return HAL_OK == HAL_RTC_GetTime(&hrtc, time, RTC_FORMAT_BIN);
}

bool clock_get_rtc_date(RTC_DateTypeDef* date)
{
	return HAL_OK == HAL_RTC_GetDate(&hrtc, date, RTC_FORMAT_BIN);
}


uint32_t clock_datetime_to_seconds(const RTC_DateTypeDef* date, const RTC_TimeTypeDef* time)
{
	uint32_t days = date->Year * DAYS_PER_YEAR;
	if (date->Year > 0) {
		days += (uint32_t)((date->Year - 1) / LEAP_YEAR_PERIOD) + 1;
	}
	for (unsigned i = 0; i < (unsigned)(date->Month > 0 ? date->Month - 1 : 0); i++) {
		days += _get_days_in_month(date->Year, i);
	}
	days += date->Date;
	if (!days) {
		return 0;
	}
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

void clock_seconds_to_datetime(const uint32_t seconds, RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
{
	memset(date, 0, sizeof(RTC_DateTypeDef));
	memset(time, 0, sizeof(RTC_TimeTypeDef));

	time->Seconds = (uint8_t)(seconds % SECONDS_PER_MINUTE);
	uint32_t minutes = seconds / SECONDS_PER_MINUTE;

	time->Minutes = (uint8_t)(minutes % MINUTES_PER_HOUR);
	uint32_t hours = minutes / MINUTES_PER_HOUR;

	time->Hours = (uint8_t)(hours % HOURS_PER_DAY);
	uint32_t days = 1 + hours / HOURS_PER_DAY;

	date->WeekDay = (uint8_t)((RTC_WEEKDAY_THURSDAY + days) % (DAYS_PER_WEEK)) + 1;
	date->Month = 1;
	while (days) {
		uint16_t days_in_year = (date->Year % LEAP_YEAR_PERIOD > 0) ? DAYS_PER_YEAR : DAYS_PER_LEAP_YEAR;
		if (days >= days_in_year) {
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

char* get_clock_time_format()
{
	static char format_time[30] = "";
	memset(format_time, '-', sizeof(format_time) - 1);
	format_time[sizeof(format_time) - 1] = 0;

	RTC_DateTypeDef date = {0};
	RTC_TimeTypeDef time = {0};

	if (!clock_get_rtc_date(&date)) {
		BEDUG_ASSERT(false, "Unable to get current date");
		memset((void*)&date, 0, sizeof(date));
		return format_time;
	}

	if (!clock_get_rtc_time(&time)) {
		BEDUG_ASSERT(false, "Unable to get current time");
		memset((void*)&time, 0, sizeof(time));
		return format_time;
	}

	snprintf(
		format_time,
		sizeof(format_time) - 1,
		"20%02u-%02u-%02uT%02u:%02u:%02u",
		date.Year,
		date.Month,
		date.Date,
		time.Hours,
		time.Minutes,
		time.Seconds
	);

	return format_time;
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
