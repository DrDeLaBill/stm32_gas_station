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

const bool error_arr[][sizeof(indicators_pins)] = {
/*   A  B  C  D  E  F  G    */
	{1, 0, 0, 1, 1, 1, 1},  // E
	{0, 0, 0, 0, 1, 0, 1},  // r
	{0, 0, 0, 0, 1, 0, 1},  // r
	{0, 0, 0, 0, 0, 0, 0},  // empty
	{0, 0, 0, 0, 0, 0, 0}   // empty
};


#define INDICATE_INDICATORS_COUNT  ((uint8_t)sizeof(indicators_pins))
#define INDICATE_SHOW_DELAY_MS     ((uint8_t)50)
#define INDICATE_FSM_WAIT_DELAY_MS ((uint8_t)1000)
#define INDICATE_FSM_LOAD_DELAY_MS ((uint8_t)300)
#define INDICATE_DIGITS_PORT       ((GPIO_TypeDef*)DIGITS_1_GPIO_Port)


typedef struct _indicate_fsm_state_t {
	void         (*indicate_state) (void);
	uint8_t      indicator_idx;
	util_timer_t show_timer;
	util_timer_t wait_timer;
	uint8_t      indicate_buffer[INDICATE_INDICATORS_COUNT];
	uint8_t      load_segment_num;
} indicate_state_t;


void _indicate_fsm_indicator_idx_update();

void _indicate_fsm_wait();
void _indicate_fsm_buffer();
void _indicate_fsm_load();
void _indicate_fsm_error();


indicate_state_t indicate_state = {
	.indicate_state  = _indicate_fsm_wait,
	.indicator_idx   = 0,
	.wait_timer      = { 0 },
	.indicate_buffer = { 0 },
};


void indicate_set_buffer(uint8_t* data, uint8_t len)
{
	if (len > __arr_len(indicate_state.indicate_buffer)) {
		len = __arr_len(indicate_state.indicate_buffer);
	}
	memcpy(indicate_state.indicate_buffer, data, len);
}

void indicate_proccess()
{
	if (util_is_timer_wait(&indicate_state.show_timer)) {
		return;
	}

	HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, DIGITS_DP_Pin, GPIO_PIN_RESET);

	util_timer_start(&indicate_state.show_timer, INDICATE_SHOW_DELAY_MS);

	if (indicate_state.indicate_buffer[indicate_state.indicator_idx] > __arr_len(digits_pins) - 1) {
		indicate_state.indicate_buffer[indicate_state.indicator_idx] = 0;
	}

	indicate_state.indicate_state();

	_indicate_fsm_indicator_idx_update();
}

void indicate_set_wait_page()
{
	indicate_state.indicate_state = &_indicate_fsm_wait;
}

void indicate_set_buffer_page()
{
	indicate_state.indicate_state = &_indicate_fsm_buffer;
}

void indicate_set_load_page()
{
	indicate_state.indicate_state = &_indicate_fsm_load;
}

void indicate_set_error_page()
{
	indicate_state.indicate_state = &_indicate_fsm_error;
}

void _indicate_fsm_indicator_idx_update()
{
	indicate_state.indicator_idx = (indicate_state.indicator_idx + 1) % __arr_len(indicators_pins);
}

void _indicate_fsm_wait()
{
	if (util_is_timer_wait(&indicate_state.wait_timer)) {
		return;
	}

	for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
		HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, segments_pins[i], GPIO_PIN_RESET);
	}
	HAL_GPIO_TogglePin(INDICATE_DIGITS_PORT, segments_pins[__arr_len(segments_pins) - 1]);

	if (indicate_state.indicator_idx >= __arr_len(indicators_pins)) {
		util_timer_start(&indicate_state.wait_timer, INDICATE_FSM_WAIT_DELAY_MS);
	}
}

void _indicate_fsm_buffer()
{
	for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
		uint8_t number = indicate_state.indicate_buffer[indicate_state.indicator_idx];

		HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, segments_pins[i], digits_pins[number][i]);

		if (i == __arr_len(segments_pins) - 2) {
			HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, DIGITS_DP_Pin, GPIO_PIN_SET);
		}
	}
}

void _indicate_fsm_load()
{
	if (util_is_timer_wait(&indicate_state.wait_timer)) {
		return;
	}

	for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
		GPIO_PinState state = GPIO_PIN_RESET;
		if (i == indicate_state.load_segment_num) {
			state = GPIO_PIN_SET;
		}

		HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, segments_pins[i], state);
	}

	if (indicate_state.indicator_idx >= __arr_len(indicators_pins) - 1) {
		indicate_state.load_segment_num += 1;
		indicate_state.load_segment_num %= __arr_len(segments_pins);
	}

	util_timer_start(&indicate_state.wait_timer, INDICATE_FSM_LOAD_DELAY_MS);
}

void _indicate_fsm_error()
{
	for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
		HAL_GPIO_WritePin(INDICATE_DIGITS_PORT, segments_pins[i], error_arr[indicate_state.indicator_idx][i]);
	}
}
