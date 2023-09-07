/* Copyright © 2023 Georgy E. All rights reserved. */

#include "clock.h"

#include <stdint.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "main.h"
#include "utils.h"


const char* CLOCK_TAG = "CLCK";


uint8_t clock_get_year()
{
	RTC_DateTypeDef date;
	if (HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return date.Year;
}

uint8_t clock_get_month()
{
	RTC_DateTypeDef date;
	if (HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return date.Month;
}

uint8_t clock_get_date()
{
	RTC_DateTypeDef date;
	if (HAL_RTC_GetDate(&CLOCK_RTC, &date, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return date.Date;
}

uint8_t clock_get_hour()
{
	RTC_TimeTypeDef time;
	if (HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return time.Hours;
}

uint8_t clock_get_minute()
{
	RTC_TimeTypeDef time;
	if (HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return time.Minutes;
}

uint8_t clock_get_second()
{
	RTC_TimeTypeDef time;
	if (HAL_RTC_GetTime(&CLOCK_RTC, &time, RTC_FORMAT_BCD) != HAL_OK)
	{
	    return 0;
	}
	return time.Seconds;
}

void clock_save_time(RTC_TimeTypeDef* time)
{
	if (HAL_RTC_SetTime(&CLOCK_RTC, time, RTC_FORMAT_BCD) != HAL_OK)
	{
	    LOG_TAG_BEDUG(CLOCK_TAG, "time was not saved");
	}
}

void clock_save_date(RTC_DateTypeDef* date)
{
	if (HAL_RTC_SetDate(&CLOCK_RTC, date, RTC_FORMAT_BCD) != HAL_OK)
	{
	    LOG_TAG_BEDUG(CLOCK_TAG, "date was not saved");
	}
}
