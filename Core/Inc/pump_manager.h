/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _PUMP_MANAGER_H_
#define _PUMP_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#define PUMP_BEDUG (true)


void pump_proccess();

void pump_set_fuel_ml(uint32_t liquid_ml);
void pump_start();
void pump_stop();

bool pump_has_error();
bool pump_is_working();
bool pump_is_free();

void pump_set_record_handler(void (*pump_record_handler) (void));

uint32_t pump_get_fuel_count_ml();


#ifdef __cplusplus
}
#endif


#endif
