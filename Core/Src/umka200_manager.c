/* Copyright © 2023 Georgy E. All rights reserved. */

#include <umka200_manager.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"


#define UMKA200_DEFAULT_DELAY_MS     ((uint32_t)1000)
#define UMKA200_READ_DELAY_MS        ((uint32_t)2000)

#define UMKA200_MESSAGE_ID_DEFAULT   ((uint8_t)0x01)
#define UMKA200_MESSAGE_PREFIX       ((uint8_t)0xC8)
#define UMKA200_COMMAND_MASK         ((uint8_t)0x80)
#define UMKA200_NO_ERROR             ((uint8_t)0x00)

#define UMKA200_COMMAND_GET_STATE    ((uint8_t)0x06)
#define UMKA200_SUBCOMMAND_GET_STATE ((uint8_t)0x02)

#define UMKA200_COMMAND_GET_RFID     ((uint8_t)0x09)
#define UMKA200_SUBCOMMAND_GET_RFID  ((uint8_t)0x00)


typedef struct _umka200_message_t {
	uint8_t id;
	uint8_t length;
	uint8_t prefix;
	uint8_t command;
	uint8_t subcommand;
	uint8_t data[UMKA200_MESSAGE_DATA_SIZE];
	uint8_t crc;
} umka200_message_t;

typedef struct _umka200_state_t {
	void              (*byte_proccess_handler) (uint8_t byte);
	umka200_message_t response;
	uint16_t          payload_counter;
	bool              is_success_response;

	umka200_message_t request;
	void              (*data_sender_handler)   (uint8_t* data, uint16_t len);
	void              (*request_proccess)      (void);
	timer_t           request_timer;
	uint32_t          current_rfid;
} umka200_state_t;


void _umka200_fsm_response_id(uint8_t byte);
void _umka200_fsm_response_length(uint8_t byte);
void _umka200_fsm_response_prefix(uint8_t byte);
void _umka200_fsm_response_command(uint8_t byte);
void _umka200_fsm_response_subcommand(uint8_t byte);
void _umka200_fsm_response_data(uint8_t byte);
void _umka200_fsm_response_crc(uint8_t byte);

void _umka200_fsm_request_start();
void _umka200_fsm_request_get_state();
void _umka200_fsm_request_wait_state();
void _umka200_fsm_request_get_rfid();
void _umka200_fsm_request_wait_rfid();
void _umka200_fsm_request_wait_read();

void _umka200_generate_command(uint8_t* data, uint8_t command, uint8_t subcommand, uint16_t* counter);
void _umka200_response_proccess();
void _umka200_reset_state();

uint8_t _umka200_get_crc(const uint8_t *data, uint16_t len);


const char* UMKA200_TAG = "RFID";

umka200_state_t umka200_state = {
	.byte_proccess_handler = _umka200_fsm_response_id,
	.response              = { 0 },
	.payload_counter       = 0,
	.is_success_response   = false,

	.request               = { 0 },
	.data_sender_handler   = NULL,
	.request_proccess      = _umka200_fsm_request_start,
	.request_timer         = { 0 },
	.current_rfid          = 0
};


void umka200_proccess()
{
	if (umka200_state.request_proccess != NULL) {
		umka200_state.request_proccess();
	} else {
		_umka200_reset_state();
	}
}

uint32_t umka200_get_rfid()
{
	uint32_t tmp_rfid = umka200_state.current_rfid;
	umka200_state.current_rfid = 0;
	return tmp_rfid;
}

void umka200_recieve_byte(uint8_t byte)
{
	if (umka200_state.byte_proccess_handler != NULL) {
		umka200_state.byte_proccess_handler(byte);
	}
}

void umka200_set_data_sender(void (*data_sender_handler) (uint8_t* data, uint16_t len))
{
	if (data_sender_handler != NULL) {
		umka200_state.data_sender_handler = data_sender_handler;
	}
}

void umka200_timeout()
{
	_umka200_reset_state();
}

void _umka200_generate_command(uint8_t* data, uint8_t command, uint8_t subcommand, uint16_t* counter)
{
	uint16_t tmp_counter = 0;

	umka200_state.request.id         = UMKA200_MESSAGE_ID_DEFAULT;
	data[tmp_counter++]              = UMKA200_MESSAGE_ID_DEFAULT;

	umka200_state.request.length     = UMKA200_MESSAGE_META_SIZE;
	data[tmp_counter++]              = UMKA200_MESSAGE_META_SIZE;

	umka200_state.request.prefix     = UMKA200_MESSAGE_PREFIX;
	data[tmp_counter++]              = UMKA200_MESSAGE_PREFIX;

	umka200_state.request.command    = command;
	data[tmp_counter++]              = command;

	umka200_state.request.subcommand = subcommand;
	data[tmp_counter++]              = subcommand;

	uint8_t crc = _umka200_get_crc(data, tmp_counter);
	umka200_state.request.crc        = crc;
	data[tmp_counter++]              = crc;

	*counter = tmp_counter;
}

void _umka200_response_proccess()
{
	if (umka200_state.response.id != UMKA200_MESSAGE_ID_DEFAULT) {
		goto do_error;
	}

	if (!umka200_state.response.length) {
		goto do_error;
	}

	if (umka200_state.response.command != (umka200_state.request.command + UMKA200_COMMAND_MASK)) {
		goto do_error;
	}

	if (umka200_state.response.subcommand != umka200_state.request.subcommand) {
		goto do_error;
	}

	if (umka200_state.response.data[0] != UMKA200_NO_ERROR) {
		goto do_error;
	}

	if (_umka200_get_crc(umka200_state.response.data, umka200_state.response.length)) {
		goto do_error;
	}

	if (umka200_state.payload_counter < 1) {
		goto do_error;
	}

	goto do_success;

do_success:
	umka200_state.is_success_response = true;

	umka200_state.current_rfid = 0;

	for (uint8_t i = 0; i < umka200_state.payload_counter - 1; i++) {
		umka200_state.current_rfid <<= 8;
		umka200_state.current_rfid  |= (uint8_t)umka200_state.response.data[1 + i];
	}

	return;

do_error:
	umka200_state.is_success_response = false;

	_umka200_reset_state();

	return;
}

void _umka200_fsm_response_id(uint8_t byte)
{
	umka200_state.response.id = byte;
	umka200_state.byte_proccess_handler = &_umka200_fsm_response_length;
}

void _umka200_fsm_response_length(uint8_t byte)
{
	umka200_state.response.length = byte;
	umka200_state.byte_proccess_handler = &_umka200_fsm_response_prefix;
}

void _umka200_fsm_response_prefix(uint8_t byte)
{
	umka200_state.response.prefix = byte;
	umka200_state.byte_proccess_handler = &_umka200_fsm_response_command;
}

void _umka200_fsm_response_command(uint8_t byte)
{
	umka200_state.response.command = byte;
	umka200_state.byte_proccess_handler = &_umka200_fsm_response_subcommand;
}

void _umka200_fsm_response_subcommand(uint8_t byte)
{
	umka200_state.response.subcommand = byte;
	umka200_state.byte_proccess_handler = &_umka200_fsm_response_data;
}

void _umka200_fsm_response_data(uint8_t byte)
{
	if (umka200_state.response.length < UMKA200_MESSAGE_META_SIZE) {
		return;
	}

	umka200_state.response.data[umka200_state.payload_counter++] = byte;

	if (umka200_state.payload_counter >= umka200_state.response.length - UMKA200_MESSAGE_META_SIZE) {
		umka200_state.byte_proccess_handler = &_umka200_fsm_response_crc;
	}
}

void _umka200_fsm_response_crc(uint8_t byte)
{
	umka200_state.response.crc = byte;
	_umka200_response_proccess();
	_umka200_reset_state();
}


void _umka200_fsm_request_start()
{
	_umka200_reset_state();
	umka200_state.request_proccess = _umka200_fsm_request_get_state;
}

void _umka200_fsm_request_get_state()
{
    uint16_t counter = 0;
    uint8_t data[UMKA200_MESSAGE_DATA_SIZE + UMKA200_MESSAGE_META_SIZE + 3 * sizeof(uint8_t)] = { 0 };

    _umka200_generate_command(data, UMKA200_COMMAND_GET_STATE, UMKA200_SUBCOMMAND_GET_STATE, &counter);

    if (umka200_state.data_sender_handler != NULL) {
    	umka200_state.data_sender_handler(data, counter);
    	umka200_state.request_proccess = _umka200_fsm_request_wait_state;
    	util_timer_start(&umka200_state.request_timer, UMKA200_DEFAULT_DELAY_MS);
    } else {
    	_umka200_reset_state();
    }
}

void _umka200_fsm_request_wait_state()
{
	if (umka200_state.is_success_response) {
    	umka200_state.request_proccess = _umka200_fsm_request_get_rfid;
    	return;
	}
	if (util_is_timer_wait(&umka200_state.request_timer)) {
		return;
	}
	_umka200_reset_state();
}

void _umka200_fsm_request_get_rfid()
{
	uint16_t counter = 0;
	uint8_t data[UMKA200_MESSAGE_DATA_SIZE + UMKA200_MESSAGE_META_SIZE + 3 * sizeof(uint8_t)] = { 0 };

    _umka200_generate_command(data, UMKA200_COMMAND_GET_RFID, UMKA200_SUBCOMMAND_GET_RFID, &counter);

	if (umka200_state.data_sender_handler != NULL) {
		umka200_state.data_sender_handler(data, counter);
    	umka200_state.request_proccess = _umka200_fsm_request_wait_rfid;
    	util_timer_start(&umka200_state.request_timer, UMKA200_DEFAULT_DELAY_MS);
    } else {
    	_umka200_reset_state();
    }
}

void _umka200_fsm_request_wait_rfid()
{
	if (umka200_state.is_success_response && umka200_state.current_rfid) {
    	util_timer_start(&umka200_state.request_timer, UMKA200_DEFAULT_DELAY_MS);
		umka200_state.request_proccess = _umka200_fsm_request_get_rfid;
		return;
	}
	if (util_is_timer_wait(&umka200_state.request_timer)) {
		return;
	}
	_umka200_reset_state();
}

void _umka200_fsm_request_wait_read()
{
	if (util_is_timer_wait(&umka200_state.request_timer)) {
		return;
	}
	_umka200_reset_state();
}

void _umka200_reset_state()
{
	memset((uint8_t*)&umka200_state.request, 0, sizeof(umka200_state.request));
	memset((uint8_t*)&umka200_state.response, 0, sizeof(umka200_state.response));
	memset((uint8_t*)&umka200_state.request_timer, 0, sizeof(umka200_state.request_timer));

	umka200_state.byte_proccess_handler = &_umka200_fsm_response_id;
	umka200_state.request_proccess      = &_umka200_fsm_request_start;
	umka200_state.payload_counter       = 0;
	umka200_state.current_rfid          = 0;
	umka200_state.is_success_response   = false;
}

uint8_t _umka200_get_crc(const uint8_t *data, uint16_t len)
{
	uint8_t crc = 0 ;
	for (uint8_t i = 0 ; i < len ; i++)
	{
		crc = crc ^ data[i];
	}
	return crc;
}
