/* Copyright © 2023 Georgy E. All rights reserved. */

#include "indicate_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


static const util_port_pin_t indicators_pins[] = { // TODO: static (check memory)
    {.port = DIGITS_6_GPIO_Port, .pin = DIGITS_6_Pin},
    {.port = DIGITS_5_GPIO_Port, .pin = DIGITS_5_Pin},
    {.port = DIGITS_4_GPIO_Port, .pin = DIGITS_4_Pin},
    {.port = DIGITS_3_GPIO_Port, .pin = DIGITS_3_Pin},
    {.port = DIGITS_2_GPIO_Port, .pin = DIGITS_2_Pin},
    {.port = DIGITS_1_GPIO_Port, .pin = DIGITS_1_Pin}
};

static const util_port_pin_t segments_pins[] = {
    {.port = DIGITS_A_GPIO_Port, .pin = DIGITS_A_Pin},
    {.port = DIGITS_B_GPIO_Port, .pin = DIGITS_B_Pin},
    {.port = DIGITS_C_GPIO_Port, .pin = DIGITS_C_Pin},
    {.port = DIGITS_D_GPIO_Port, .pin = DIGITS_D_Pin},
    {.port = DIGITS_E_GPIO_Port, .pin = DIGITS_E_Pin},
    {.port = DIGITS_F_GPIO_Port, .pin = DIGITS_F_Pin},
    {.port = DIGITS_G_GPIO_Port, .pin = DIGITS_G_Pin},
};

static const bool digits_pins[][__arr_len(segments_pins)] = {
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

static const bool symbol_underline[__arr_len(segments_pins)] =
/*   A  B  C  D  E  F  G    */
	{0, 0, 0, 1, 0, 0, 0};

static const bool error_arr[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
    {1, 0, 0, 1, 1, 1, 1},  // E
    {0, 0, 0, 0, 1, 0, 1},  // r
    {0, 0, 0, 0, 1, 0, 1},  // r
    {1, 1, 1, 1, 1, 1, 0},  // O
    {0, 0, 0, 0, 1, 0, 1},  // r
    {0, 0, 0, 0, 0, 0, 0}   // empty
};

static const bool limit_arr[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
    {0, 0, 0, 1, 1, 1, 0},  // L
    {0, 0, 0, 0, 1, 0, 0},  // i_
    {1, 1, 0, 0, 1, 1, 0},  // M_
    {1, 1, 1, 0, 0, 1, 0},  // _M
    {0, 0, 1, 0, 0, 0, 0},  // _i
    {0, 0, 0, 1, 1, 1, 1}   // t
};

static const bool access_arr[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
	{1, 1, 1, 0, 1, 1, 1},  // A
    {0, 0, 0, 1, 1, 0, 1},  // c
    {0, 0, 0, 1, 1, 0, 1},  // c
    {1, 0, 0, 1, 1, 1, 1},  // E
    {1, 0, 1, 1, 0, 1, 1},  // S
    {1, 0, 1, 1, 0, 1, 1}   // S
};

static const bool denied_arr[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
	{0, 1, 1, 1, 1, 0, 1},  // d
    {1, 0, 0, 1, 1, 1, 1},  // E
    {0, 0, 1, 0, 1, 0, 1},  // n
    {0, 0, 1, 0, 0, 0, 0},  // _i
    {1, 0, 0, 1, 1, 1, 1},  // E
	{0, 1, 1, 1, 1, 0, 1}   // d
};

static const bool reboot_arr[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
	{0, 0, 0, 0, 1, 0, 1},  // r
	{1, 0, 0, 1, 1, 1, 1},  // E
	{0, 0, 1, 1, 1, 1, 1},  // b
	{1, 1, 1, 1, 1, 1, 0},  // O
	{1, 1, 1, 1, 1, 1, 0},  // O
    {0, 0, 0, 1, 1, 1, 1}   // t
};

#define INDICATE_INDICATORS_COUNT  ((uint32_t)__arr_len(indicators_pins))
#define INDICATE_SHOW_DELAY_MS     ((uint32_t)50)
#define INDICATE_FSM_WAIT_DELAY_MS ((uint32_t)1000)
#define INDICATE_FSM_LOAD_DELAY_MS ((uint32_t)120)


typedef struct _indicate_fsm_state_t {
    void             (*indicate_state) (void);
    util_old_timer_t wait_timer;
    uint8_t          indicate_buffer[INDICATE_INDICATORS_COUNT];
    uint8_t          load_segment_num;
} indicate_state_t;

bool display_buffer[][__arr_len(segments_pins)] = {
/*   A  B  C  D  E  F  G    */
    {0, 0, 0, 0, 0, 0, 0},  // DIGIT_1
    {0, 0, 0, 0, 0, 0, 0},  // DIGIT_2
    {0, 0, 0, 0, 0, 0, 0},  // DIGIT_3
    {0, 0, 0, 0, 0, 0, 0},  // DIGIT_4
    {0, 0, 0, 0, 0, 0, 0},  // DIGIT_5
    {0, 0, 0, 0, 0, 0, 0}   // DIGIT_6
};


void _indicate_clear_buffer();

void _indicate_fsm_wait();
void _indicate_fsm_reboot();
void _indicate_fsm_buffer();
void _indicate_fsm_blink_buffer();
void _indicate_fsm_load();
void _indicate_fsm_limit();
void _indicate_fsm_access();
void _indicate_fsm_denied();
void _indicate_fsm_error();

void _indicate_set_page(void (*new_page) (void));

void _indicate_display();


indicate_state_t indicate_state = {
    .indicate_state  = _indicate_fsm_wait,
    .wait_timer      = { 0 },
    .indicate_buffer = { 0 },
};


void indicate_set_buffer(uint8_t* data, uint8_t len)
{
    if (len > __arr_len(indicate_state.indicate_buffer)) {
        len = __arr_len(indicate_state.indicate_buffer);
    }
    for (uint8_t i = 0; i < len; i++) {
    	indicate_state.indicate_buffer[i] = data[i];
    }
}

void indicate_clear_buffer()
{
    memset(indicate_state.indicate_buffer, 0, sizeof(indicate_state.indicate_buffer));
}

void indicate_proccess()
{
    indicate_state.indicate_state();

    _indicate_display();
}

void _indicate_display()
{
    for (uint8_t i = 0; i < __arr_len(indicators_pins); i++) {
        HAL_GPIO_WritePin(indicators_pins[i].port, indicators_pins[i].pin, GPIO_PIN_RESET);
    }

    static uint8_t curr_indicator_idx = 0;

    HAL_GPIO_WritePin(indicators_pins[curr_indicator_idx].port, indicators_pins[curr_indicator_idx].pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(DIGITS_DP_GPIO_Port, DIGITS_DP_Pin, GPIO_PIN_RESET);

    for (uint8_t i = 0; i < __arr_len(segments_pins); i++) {
        HAL_GPIO_WritePin(
            segments_pins[i].port,
            segments_pins[i].pin,
            display_buffer[curr_indicator_idx][i]
        );
    }

    if ((indicate_state.indicate_state == _indicate_fsm_buffer ||
		indicate_state.indicate_state == _indicate_fsm_blink_buffer) &&
		curr_indicator_idx == __arr_len(indicators_pins) - 3
	) {
        HAL_GPIO_WritePin(DIGITS_DP_GPIO_Port, DIGITS_DP_Pin, GPIO_PIN_SET);
    }

    curr_indicator_idx++;
    if (curr_indicator_idx >= __arr_len(indicators_pins)) {
        curr_indicator_idx = 0;
    }
}

void _indicate_set_page(void (*new_page) (void))
{
    if (new_page == indicate_state.indicate_state) {
        return;
    }
    if (!new_page) {
        return;
    }
    _indicate_clear_buffer();
    indicate_state.indicate_state = new_page;
}

void indicate_set_wait_page()
{
    _indicate_set_page(&_indicate_fsm_wait);
}

void indicate_set_reboot_page()
{
    _indicate_set_page(&_indicate_fsm_reboot);
}

void indicate_set_buffer_page()
{
    _indicate_set_page(&_indicate_fsm_buffer);
}

void indicate_set_blink_buffer_page()
{
	_indicate_set_page(&_indicate_fsm_blink_buffer);
}

void indicate_set_load_page()
{
    _indicate_set_page(&_indicate_fsm_load);
}

void indicate_set_limit_page()
{
    _indicate_set_page(&_indicate_fsm_limit);
}

void indicate_set_access_page()
{
    _indicate_set_page(&_indicate_fsm_access);
}

void indicate_set_denied_page()
{
    _indicate_set_page(&_indicate_fsm_denied);
}

void indicate_set_error_page()
{
    _indicate_set_page(&_indicate_fsm_error);
}

void _indicate_clear_buffer()
{
    memset(display_buffer, 0, sizeof(display_buffer));
}

void _indicate_fsm_wait()
{
    if (util_old_timer_wait(&indicate_state.wait_timer)) {
        return;
    }

    for (uint8_t i = 0; i < __arr_len(indicators_pins); i++) {
        display_buffer[i][__arr_len(segments_pins) - 1] = !display_buffer[i][__arr_len(segments_pins) - 1];
    }
    util_old_timer_start(&indicate_state.wait_timer, INDICATE_FSM_WAIT_DELAY_MS);
}

void _indicate_fsm_reboot()
{
	memcpy(display_buffer, reboot_arr, sizeof(display_buffer));
}

void _indicate_fsm_buffer()
{
    for (uint8_t i = 0; i < __arr_len(indicators_pins); i++) {
        uint8_t number = indicate_state.indicate_buffer[i];
        if (number >= '0' && number <= '9') {
            memcpy(display_buffer[i], digits_pins[number - '0'], sizeof(display_buffer[i]));
        } else if (number == '_') {
            memcpy(display_buffer[i], symbol_underline, sizeof(display_buffer[i]));
        } else if (!number) {
            memset(display_buffer[i], 0, sizeof(display_buffer[i]));
        } else {
            memset(display_buffer[i], 0, sizeof(display_buffer[i]));
            display_buffer[i][__arr_len(segments_pins) - 1] = true;
        }
    }
}

void _indicate_fsm_blink_buffer()
{
	static bool empty_page = false;

    if (empty_page) {
    	memset(display_buffer, 0, sizeof(display_buffer));
    } else {
    	_indicate_fsm_buffer();
    }

    if (util_old_timer_wait(&indicate_state.wait_timer)) {
        return;
    }

    empty_page = !empty_page;

    util_old_timer_start(&indicate_state.wait_timer, INDICATE_FSM_WAIT_DELAY_MS);
}


void _indicate_fsm_load()
{
    if (util_old_timer_wait(&indicate_state.wait_timer)) {
        return;
    }

    for (uint8_t i = 0; i < __arr_len(indicators_pins); i++) {
        for (uint8_t j = 0; j < __arr_len(segments_pins); j++) {
            bool state = false;
            if (j == indicate_state.load_segment_num) {
                state = true;
            }

            display_buffer[i][j] = state;
        }
    }

    indicate_state.load_segment_num++;
    if (indicate_state.load_segment_num >= __arr_len(segments_pins) - 1) {
        indicate_state.load_segment_num = 0;
    }

    util_old_timer_start(&indicate_state.wait_timer, INDICATE_FSM_LOAD_DELAY_MS);
}

void _indicate_fsm_limit()
{
	memcpy(display_buffer, limit_arr, sizeof(display_buffer));
}

void _indicate_fsm_access()
{
	memcpy(display_buffer, access_arr, sizeof(display_buffer));
}

void _indicate_fsm_denied()
{
	memcpy(display_buffer, denied_arr, sizeof(display_buffer));
}

void _indicate_fsm_error()
{
	memcpy(display_buffer, error_arr, sizeof(display_buffer));
}
