/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _CLOCK_H_
#define _CLOCK_H_


#include <stdint.h>

#include "stm32f4xx_hal_rtc.h"


uint8_t clock_get_year();
uint8_t clock_get_month();
uint8_t clock_get_date();
uint8_t clock_get_hour();
uint8_t clock_get_minute();
uint8_t clock_get_second();
void    clock_save_time(RTC_TimeTypeDef* time);
void    clock_save_date(RTC_DateTypeDef* date);


#endif
