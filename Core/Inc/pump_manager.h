/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _PUMP_MANAGER_H_
#define _PUMP_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void     pump_set_fuel(uint32_t amount_ml);
void     pump_proccess();
uint32_t pump_get_fuel_count_ml();
bool     pump_has_error();

void     pump_set_pump_stop_handler(void (*pump_stop_handler) (void));


#ifdef __cplusplus
}
#endif


#endif
