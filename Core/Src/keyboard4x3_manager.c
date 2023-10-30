/* Copyright © 2023 Georgy E. All rights reserved. */

#include "keyboard4x3_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


#define KEYBOARD4X3_DEBOUNCE_DELAY_MS ((uint32_t)150)
#define KEYBOARD4X3_PRESS_DELAY_MS    ((uint32_t)5000)
#define KEYBOARD4X3_RESET_DELAY_MS    ((uint32_t)30000)


static const char KEYBOARD_TAG[] = "KBD";


typedef struct _keyboard4x3_state_t {
	void         (*fsm_measure_proccess) (void);
	uint8_t      cur_col;
	uint8_t      cur_row;
	uint8_t      last_row;
	util_timer_t wait_timer;
	util_timer_t reset_timer;
	uint8_t      buffer_idx;
	uint8_t      buffer[KEYBOARD4X3_BUFFER_SIZE];

	bool         cancelPressed;
	bool         enterPressed;
} keyboard4x3_state_t;


const util_port_pin_t rows_pins[] = {
	{ .port = KBD_ROW1_GPIO_Port, .pin = KBD_ROW1_Pin },
	{ .port = KBD_ROW2_GPIO_Port, .pin = KBD_ROW2_Pin },
	{ .port = KBD_ROW3_GPIO_Port, .pin = KBD_ROW3_Pin },
	{ .port = KBD_ROW4_GPIO_Port, .pin = KBD_ROW4_Pin }
};
const util_port_pin_t cols_pins[] = {
	{ .port = KBD_COL3_GPIO_Port, .pin = KBD_COL3_Pin },
	{ .port = KBD_COL2_GPIO_Port, .pin = KBD_COL2_Pin },
	{ .port = KBD_COL1_GPIO_Port, .pin = KBD_COL1_Pin }
};
const uint8_t keyboard_btns[4][3] = {
	{ '1', '2', '3' },
	{ '4', '5', '6' },
	{ '7', '8', '9' },
	{ '*', '0', '#' }
};

keyboard4x3_state_t keyboard4x3_state = {
	.buffer               = { 0 },
	.buffer_idx           = 0,
	.cur_col              = 0,
	.cur_row              = 0,
	.fsm_measure_proccess = NULL,
	.last_row             = __arr_len(rows_pins),
	.reset_timer          = { 0 },
	.wait_timer           = { 0 },

	.cancelPressed        = false,
	.enterPressed         = false,
};

void _keyboard4x3_set_output_pin(uint16_t row_num);
void _keyboard4x3_reset_buffer();

void _keyboard4x3_fsm_set_row();
void _keyboard4x3_fsm_check_button();
void _keyboard4x3_fsm_wait_button();
void _keyboard4x3_fsm_wait_debounce();
void _keyboard4x3_fsm_register_button();
void _keyboard4x3_fsm_next_button();

void _keyboard4x3_show_buf();


void keyboard4x3_proccess()
{
	if (!keyboard4x3_state.fsm_measure_proccess) {
		_keyboard4x3_reset_buffer();
	}
	keyboard4x3_state.fsm_measure_proccess();
}

uint8_t* keyboard4x3_get_buffer()
{
	return keyboard4x3_state.buffer;
}

void keyboard4x3_clear()
{
	_keyboard4x3_reset_buffer();
	_keyboard4x3_show_buf();
}

bool keyboard4x3_is_cancel()
{
	return keyboard4x3_state.cancelPressed;
}

bool keyboard4x3_is_enter()
{
	return keyboard4x3_state.enterPressed;
}

void _keyboard4x3_fsm_set_row()
{
	if (keyboard4x3_state.buffer_idx && !util_is_timer_wait(&keyboard4x3_state.reset_timer)) {
		_keyboard4x3_reset_buffer();
		_keyboard4x3_show_buf();
	}
	_keyboard4x3_set_output_pin(keyboard4x3_state.cur_row);
	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_check_button;
}

void _keyboard4x3_fsm_check_button()
{
	GPIO_PinState state = HAL_GPIO_ReadPin(cols_pins[keyboard4x3_state.cur_col].port, cols_pins[keyboard4x3_state.cur_col].pin);
	if (state == GPIO_PIN_RESET) {
		util_timer_start(&keyboard4x3_state.wait_timer, KEYBOARD4X3_PRESS_DELAY_MS);
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_wait_button;
		return;
	} else {
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_next_button;
	}
}

void _keyboard4x3_fsm_wait_button()
{
	GPIO_PinState state = HAL_GPIO_ReadPin(cols_pins[keyboard4x3_state.cur_col].port, cols_pins[keyboard4x3_state.cur_col].pin);
	if (state == GPIO_PIN_SET) {
		util_timer_start(&keyboard4x3_state.wait_timer, KEYBOARD4X3_DEBOUNCE_DELAY_MS);
		keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_wait_debounce;
		return;
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
		memcpy(keyboard4x3_state.buffer, keyboard4x3_state.buffer + 1, sizeof(keyboard4x3_state.buffer) - 1);
		keyboard4x3_state.buffer_idx = __arr_len(keyboard4x3_state.buffer) - 1;
	}

	uint8_t ch = keyboard_btns[row][col];
	if (ch == '*') {
		keyboard4x3_state.cancelPressed = true;
	} else if (ch == '#') {
		keyboard4x3_state.enterPressed = true;
	} else {
		keyboard4x3_state.buffer[keyboard4x3_state.buffer_idx++] = ch;
	}

	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_next_button;

	util_timer_start(&keyboard4x3_state.reset_timer, KEYBOARD4X3_RESET_DELAY_MS);

	_keyboard4x3_show_buf();
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

void _keyboard4x3_set_output_pin(uint16_t row_num)
{
	if (keyboard4x3_state.last_row == row_num) {
		return;
	}

	for (uint8_t i = 0; i < __arr_len(rows_pins); i++) {
		HAL_GPIO_WritePin(rows_pins[i].port, rows_pins[i].pin, GPIO_PIN_SET);
	}

	HAL_GPIO_WritePin(rows_pins[row_num].port, rows_pins[row_num].pin, GPIO_PIN_RESET);

	keyboard4x3_state.last_row = row_num;
}

void _keyboard4x3_reset_buffer()
{
	memset(keyboard4x3_state.buffer, 0, sizeof(keyboard4x3_state.buffer));
	keyboard4x3_state.buffer_idx = 0;
	keyboard4x3_state.cur_col = 0;
	keyboard4x3_state.cur_row = 0;
	util_timer_start(&keyboard4x3_state.reset_timer, 0);
	util_timer_start(&keyboard4x3_state.wait_timer, 0);
	keyboard4x3_state.fsm_measure_proccess = _keyboard4x3_fsm_set_row;
	keyboard4x3_state.last_row = __arr_len(rows_pins);
	keyboard4x3_state.enterPressed = false;
	keyboard4x3_state.cancelPressed = false;
}

void _keyboard4x3_show_buf()
{
#ifdef DEBUG
	LOG_BEDUG("%s: \t", KEYBOARD_TAG);
	for (uint8_t i = 0; i < __arr_len(keyboard4x3_state.buffer); i++) {
		LOG_BEDUG("%c ", keyboard4x3_state.buffer[i] ? keyboard4x3_state.buffer[i] : (int)'-');
	}
	if (keyboard4x3_state.cancelPressed) {
		LOG_BEDUG(" *");
	}
	if (keyboard4x3_state.enterPressed) {
		LOG_BEDUG(" #");
	}
	LOG_BEDUG("\n");
#endif
}
