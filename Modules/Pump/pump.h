/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _PUMP_H_
#define _PUMP_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define PUMP_BEDUG (1)


void pump_init();
void pump_proccess();

void set_pump_target(uint32_t);
void pump_start();
void pump_stop();

uint32_t pump_count_ml();
bool pump_stopped();


#ifdef __cplusplus
}
#endif


#endif
