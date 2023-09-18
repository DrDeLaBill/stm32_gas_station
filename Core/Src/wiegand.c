/* Copyright © 2023 Georgy E. All rights reserved. */

#include "wiegand.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "utils.h"


#define WIEGAND_MAX_BITS_COUNT ((uint8_t)32)
#define WIEGAND_MAX_PERIOD_MS  ((uint32_t)3)


typedef struct _wiegand_state_t {
    util_timer_t reset_timer;
    uint8_t      bit_counter;
    uint32_t     value;
} wiegand_state_t;

wiegand_state_t wiegand_state = {
	.reset_timer = { 0 },
	.bit_counter = 0,
	.value       = 0
};


bool wiegand_available()
{
	return wiegand_state.bit_counter >= WIEGAND_MAX_BITS_COUNT;
}

uint32_t wiegant_get_value()
{
	if (wiegand_state.bit_counter == WIEGAND_MAX_BITS_COUNT) {
		uint32_t tmp_value  = wiegand_state.value;
		wiegand_state.value = 0;
		return tmp_value;
	}
	return 0;
}

void wiegand_set_value(uint8_t value)
{
	if (util_is_timer_wait(&wiegand_state.reset_timer)) {
		memset((uint8_t*)&wiegand_state, 0, sizeof(wiegand_state));
	}

	wiegand_state.bit_counter++;

	wiegand_state.value <<= 1;
	wiegand_state.value  |= ((uint32_t)(value > 0 ? 0x01 : 0x00));

	util_timer_start(&wiegand_state.reset_timer, WIEGAND_MAX_PERIOD_MS);
}
