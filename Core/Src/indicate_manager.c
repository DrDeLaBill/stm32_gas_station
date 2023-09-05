/* Copyright © 2023 Georgy E. All rights reserved. */

#include "indicate_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


const uint16_t indicators_pins[] = {
	DIGITS_1_Pin,
	DIGITS_2_Pin,
	DIGITS_3_Pin,
	DIGITS_4_Pin,
	DIGITS_5_Pin
};

const uint16_t segments_pins[] = {
	DIGITS_A_Pin,
	DIGITS_B_Pin,
	DIGITS_C_Pin,
	DIGITS_D_Pin,
	DIGITS_E_Pin,
	DIGITS_F_Pin,
	DIGITS_G_Pin
};

const bool digits_pins[][sizeof(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
	{1, 1, 1, 1, 1, 1, 0},  // 0
	{0, 1, 1, 0, 0, 0, 0},  // 1
	{1, 1, 0, 1, 1, 0, 1},  // 2
	{1, 1, 1, 1, 0, 0, 1},  // 3
	{0, 1, 1, 0, 0, 1, 1},  // 4
	{1, 0, 1, 1, 0, 1, 1},  // 5
	{1, 0, 1, 1, 1, 1, 1},  // 6
	{1, 1, 1, 0, 0, 0, 0},  // 7
	{1, 1, 1, 1, 1, 1, 1},  // 8
	{1, 1, 1, 1, 0, 1, 1}   // 9
};


#define INDICATE_INDICATORS_COUNT ((uint8_t)sizeof(indicators_pins))
#define INDICATE_WAIT_DELAY_MS    ((uint8_t)50)
#define INDICATE_DIGITS_PORT      ((GPIO_TypeDef*)DIGITS_1_GPIO_Port)


typedef struct _indicate_state_t {
	uint8_t      indicator_idx;
	util_timer_t wait_timer;
	uint8_t      indicate_buffer[INDICATE_INDICATORS_COUNT];
} indicate_state_t;

indicate_state_t indicate_state = { 0 };


void _indicate_reset_state();


void indicate_set_buffer(uint8_t* data, uint8_t len)
{
	if (len > __arr_len(indicate_state.indicate_buffer)) {
		len = __arr_len(indicate_state.indicate_buffer);
	}
	memcpy(indicate_state.indicate_buffer, data, len);
}

void indicate_proccess()
{
	if (util_is_timer_wait(&indicate_state.wait_timer)) {
		return;
	}

	for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
		HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, segments_pins[i], digits_pins[indicate_state.indicator_idx][i]);
		if (i == __arr_len(segments_pins) - 2) {
			HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, DIGITS_DP_Pin, GPIO_PIN_SET);
		}
	}

	indicate_state.indicator_idx = (indicate_state.indicator_idx + 1) % __arr_len(indicators_pins);

	util_timer_start(&indicate_state.wait_timer, INDICATE_WAIT_DELAY_MS);
}

void _indicate_reset_state()
{
	memset((uint8_t*)&indicate_state, 0, sizeof(indicate_state));
}
