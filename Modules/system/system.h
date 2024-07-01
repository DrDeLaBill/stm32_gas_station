/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SYSTEM_H_
#define _SYSTEM_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>

#include "soul.h"


extern uint16_t SYSTEM_ADC_VOLTAGE;


void system_clock_hsi_config(void);

void system_pre_load(void);
void system_post_load(void);

void system_error_handler(SOUL_STATUS error);


#ifdef __cplusplus
}
#endif


#endif
