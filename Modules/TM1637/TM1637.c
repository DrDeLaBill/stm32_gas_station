#include "TM1637.h"

#include "log.h"
#include "utils.h"
#include "bmacro.h"
#include "hal_defs.h"


#define CMD_WR_DATA_IDX       (0)
#define CMD_SET_ADDR0_IDX     (CMD_WR_DATA_IDX + 1)
#define CMD_DISPLAY_14_16_IDX (CMD_SET_ADDR0_IDX + NUM_OF_DIGITS + 1)


#define CLK_SET()    HAL_GPIO_WritePin(clk.port, clk.pin, GPIO_PIN_SET);
#define CLK_RESET()  HAL_GPIO_WritePin(clk.port, clk.pin, GPIO_PIN_RESET);
#define DATA_SET()   HAL_GPIO_WritePin(data.port, data.pin, GPIO_PIN_SET);
#define DATA_RESET() HAL_GPIO_WritePin(data.port, data.pin, GPIO_PIN_RESET);
#define DATA_READ()  HAL_GPIO_ReadPin(data.port, data.pin)


void _fsm_start();
void _fsm_idle();
void _fsm_start_bit_begin();
void _fsm_start_bit_end();
void _fsm_data_bit_start();
void _fsm_data_clk_start();
void _fsm_data_iterate();
void _fsm_data_end();
void _fsm_ack_start();
void _fsm_ack_set_clk();
void _fsm_ack_end();
void _fsm_stop_bit_begin();
void _fsm_stop_bit_stop();
void _fsm_ack_check();
void _fsm_iterate_data();
void _fsm_stop();


typedef struct _tm1637_state_t {
	void    (*fsm) (void);
	uint8_t bit_idx;
	uint8_t data_idx;
	bool    need_show;
	uint8_t buffer   [3 + NUM_OF_DIGITS];
	bool    start_bit[3 + NUM_OF_DIGITS];
	bool    stop_bit [3 + NUM_OF_DIGITS];
} tm1637_state_t;


GPIO_PAIR clk  = {0};
GPIO_PAIR data = {0};
tm1637_state_t state = {
	.fsm       = _fsm_start,
	.bit_idx   = 0,
	.data_idx  = 0,
	.need_show = true,
	.buffer    = {0},
	.start_bit = {0},
	.stop_bit  = {0}
};


void tm1637_init(GPIO_PAIR* clk_ptr, GPIO_PAIR* data_ptr)
{
	clk.port  = clk_ptr->port;
	clk.pin   = clk_ptr->pin;
	data.port = data_ptr->port;
	data.pin  = data_ptr->pin;

	GPIO_InitTypeDef GPIO_InitStruct = {0};

	CLK_RESET();
	GPIO_InitStruct.Pin   = clk.pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(clk.port, &GPIO_InitStruct);

	DATA_RESET();
	GPIO_InitStruct.Pin   = data.pin;
	GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull  = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(data.port, &GPIO_InitStruct);

	CLK_SET();
	DATA_SET();
}


void tm1637_proccess()
{
	if (!state.fsm) {
		BEDUG_ASSERT(false, "TM1637 was not initialized");
		return;
	}
	state.fsm();
}

void tm1637_set_buffer(uint8_t data[NUM_OF_DIGITS])
{
	for (unsigned i = 0; i < NUM_OF_DIGITS; i++) {
		state.buffer[CMD_SET_ADDR0_IDX + i + 1]    = data[i];
		state.start_bit[CMD_SET_ADDR0_IDX + i + 1] = false;
		state.stop_bit[CMD_SET_ADDR0_IDX + i + 1]  = false;
	}
	state.stop_bit[CMD_DISPLAY_14_16_IDX - 1] = true;
	state.need_show = true;
}

void tm1637_set_dot(uint8_t index, bool enable)
{
	if (index >= NUM_OF_DIGITS) {
		return;
	}
	if (enable) {
		state.buffer[index] |= ((uint8_t)DOT_ENABLE);
	} else {
		state.buffer[index] &= ((uint8_t)~DOT_ENABLE);
	}
}


void _fsm_start()
{
	memset((void*)&state, 0, sizeof(state));

	state.buffer[CMD_WR_DATA_IDX]              = CMD_WR_DATA;
	state.start_bit[CMD_WR_DATA_IDX]           = false;
	state.stop_bit[CMD_WR_DATA_IDX]            = true;

	state.buffer[CMD_SET_ADDR0_IDX]            = CMD_SET_ADDR0;
	state.start_bit[CMD_SET_ADDR0_IDX]         = true;

	state.stop_bit[CMD_DISPLAY_14_16_IDX - 1]  = true;

	state.buffer[CMD_DISPLAY_14_16_IDX]        = CMD_DISPLAY_14_16;
	state.start_bit[CMD_DISPLAY_14_16_IDX]     = true;
	state.stop_bit[CMD_DISPLAY_14_16_IDX]      = true;

    CLK_SET();
    DATA_SET();

	state.fsm = _fsm_idle;
}

void _fsm_idle()
{
	if (state.need_show) {
	    CLK_RESET();
	    DATA_RESET();
		state.fsm = _fsm_start_bit_begin;
	}
}

void _fsm_start_bit_begin()
{
	if (state.start_bit[state.data_idx]) {
		CLK_SET();
		DATA_SET();

		state.fsm = _fsm_start_bit_end;
	} else {
	    DATA_RESET();
	    CLK_RESET();

		state.fsm = _fsm_data_bit_start;
	}
}

void _fsm_start_bit_end()
{
    DATA_RESET();
    CLK_RESET();

	state.fsm = _fsm_data_bit_start;
}

void _fsm_data_bit_start()
{
    CLK_RESET();

	if ((state.buffer[state.data_idx] >> state.bit_idx) & 0x01) {
	    DATA_SET();
	} else {
	    DATA_RESET();
	}

	state.fsm = _fsm_data_clk_start;
}

void _fsm_data_clk_start()
{
    CLK_SET();

	state.fsm = _fsm_data_iterate;
}

void _fsm_data_iterate()
{
	state.bit_idx++;
	if (state.bit_idx >= BITS_IN_BYTE) {
	    CLK_RESET();
		state.fsm = _fsm_data_end;
	} else {
		state.fsm = _fsm_data_bit_start;
	}
}

void _fsm_data_end()
{
	DATA_RESET();

	state.fsm = _fsm_ack_start;
}

void _fsm_ack_start()
{
    CLK_RESET();

	state.fsm = _fsm_ack_set_clk;
}

void _fsm_ack_set_clk()
{
    CLK_SET();

	state.fsm = _fsm_ack_end;
}

void _fsm_ack_end()
{
	CLK_RESET();

	state.fsm = _fsm_stop_bit_begin;
}

void _fsm_stop_bit_begin()
{
	if (state.stop_bit[state.data_idx]) {
		DATA_SET();

		state.fsm = _fsm_stop_bit_stop;
	} else {
		state.fsm = _fsm_iterate_data;
	}
}

void _fsm_stop_bit_stop()
{
	DATA_RESET();

	state.fsm = _fsm_iterate_data;
}

void _fsm_iterate_data()
{
	state.data_idx++;
	state.bit_idx = 0;
	if (state.data_idx >= __arr_len(state.buffer)) {
		state.fsm = _fsm_stop;
	} else {
		state.fsm = _fsm_start_bit_begin;
	}
}

void _fsm_stop()
{
    CLK_SET();
    DATA_SET();
	state.fsm = _fsm_start;
}
