/* Copyright © 2023 Georgy E. All rights reserved. */

#include "keyboard4x3_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


#define KEYBOARD4X3_DEBOUNCE_DELAY_MS ((uint32_t)50)
#define KEYBOARD4X3_PRESS_DELAY_MS    ((uint32_t)5000)
#define KEYBOARD4X3_RESET_DELAY_MS    ((uint32_t)10000)
#define KEYBOARD4x3_PINS_PORT         ((GPIO_TypeDef*)KBD_COL1_GPIO_Port)


typedef struct _keyboard4x3_state_t {
	void         (*fsm_measure_proccess) (void);
	uint8_t      cur_col;
	uint8_t      cur_row;
	util_timer_t wait_timer;
	util_timer_t reset_timer;
	uint8_t      buffer_idx;
	uint8_t      buffer[KEYBOARD4X3_BUFFER_SIZE];
} keyboard4x3_state_t;


const uint16_t rows_pins[] = {
	KBD_ROW1_Pin,
	KBD_ROW2_Pin,
	KBD_ROW3_Pin,
	KBD_ROW4_Pin
};
const uint16_t cols_pins[] = {
	KBD_COL1_Pin,
	KBD_COL2_Pin,
	KBD_COL3_Pin
};
const uint8_t keyboard_btns[sizeof(rows_pins)][sizeof(cols_pins)] = {
	{ '1', '2', '3' },
	{ '4', '5', '6' },
	{ '7', '8', '9' },
	{ '*', '0', '#' }
};

keyboard4x3_state_t keyboard4x3_state = { 0 };

void _keyboard4x3_set_output_pin(uint16_t pin);
void _keyboard4x3_reset_buffer();

void _keyboard4x3_fsm_set_row();
void _keyboard4x3_fsm_check_button();
void _keyboard4x3_fsm_wait_button();
void _keyboard4x3_fsm_wait_debounce();
void _keyboard4x3_fsm_register_button();
void _keyboard4x3_fsm_next_button();

void keyboard4x3_proccess()
{
	keyboard4x3_state.fsm_measure_proccess();
}

uint8_t get_buffer_value(uint8_t idx)
{
	if (idx >= __arr_len(keyboard4x3_state.buffer)) {
		return 0;
	}
	return keyboard4x3_state.buffer[idx];
}

void _keyboard4x3_fsm_set_row()
{
	if (keyboard4x3_state.buffer_idx && util_is_timer_wait(&keyboard4x3_state.reset_timer)) {
		_keyboard4x3_reset_buffer();
	}
	_keyboard4x3_set_output_pin(rows_pins[keyboard4x3_state.cur_row]);
	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_check_button;
}

void _keyboard4x3_fsm_check_button()
{
	GPIO_PinState state = HAL_GPIO_ReadPin(KEYBOARD4x3_PINS_PORT, cols_pins[keyboard4x3_state.cur_col]);
	if (state == GPIO_PIN_SET) {
		util_timer_start(&keyboard4x3_state.reset_timer, KEYBOARD4X3_RESET_DELAY_MS);
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_wait_button;
	} else {
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_next_button;
	}
}

void _keyboard4x3_fsm_wait_button()
{
	GPIO_PinState state = HAL_GPIO_ReadPin(KEYBOARD4x3_PINS_PORT, cols_pins[keyboard4x3_state.cur_col]);
	if (state == GPIO_PIN_RESET) {
		util_timer_start(&keyboard4x3_state.wait_timer, KEYBOARD4X3_DEBOUNCE_DELAY_MS);
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_wait_debounce;
	}
	if (!util_is_timer_wait(&keyboard4x3_state.wait_timer)) {
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_next_button;
	}
}

void _keyboard4x3_fsm_wait_debounce()
{
	if (!util_is_timer_wait(&keyboard4x3_state.wait_timer)) {
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_register_button;
	}
}

void _keyboard4x3_fsm_register_button()
{
	uint8_t col = keyboard4x3_state.cur_col;
	uint8_t row = keyboard4x3_state.cur_row;

	if (keyboard4x3_state.buffer_idx >= __arr_len(keyboard4x3_state.buffer)) {
		_keyboard4x3_reset_buffer();
	}

	keyboard4x3_state.buffer[keyboard4x3_state.buffer_idx++] = keyboard_btns[row][col];

	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_next_button;

	util_timer_start(&keyboard4x3_state.wait_timer, KEYBOARD4X3_RESET_DELAY_MS);
}

void _keyboard4x3_fsm_next_button()
{
	keyboard4x3_state.cur_col++;
	if (keyboard4x3_state.cur_col >= __arr_len(cols_pins)) {
		keyboard4x3_state.cur_col = 0;
		keyboard4x3_state.cur_row++;
	}
	if (keyboard4x3_state.cur_row >= __arr_len(rows_pins)) {
		keyboard4x3_state.cur_col = 0;
		keyboard4x3_state.cur_row = 0;
	}
	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_set_row;
}

void _keyboard4x3_set_output_pin(uint16_t pin)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin  = KBD_ROW3_Pin | KBD_ROW4_Pin | KBD_COL1_Pin | KBD_COL2_Pin
						 | KBD_COL3_Pin | KBD_ROW1_Pin | KBD_ROW2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	HAL_GPIO_Init(KEYBOARD4x3_PINS_PORT, &GPIO_InitStruct);

	GPIO_InitStruct.Pin   = pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(KEYBOARD4x3_PINS_PORT, &GPIO_InitStruct);

	HAL_GPIO_WritePin(KEYBOARD4x3_PINS_PORT, pin, GPIO_PIN_SET);
}

void _keyboard4x3_reset_buffer()
{
	memset((uint8_t*)&keyboard4x3_state, 0, sizeof(keyboard4x3_state));
	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_set_row;
}
