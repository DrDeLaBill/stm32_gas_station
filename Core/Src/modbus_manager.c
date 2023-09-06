/* Copyright © 2023 Georgy E. All rights reserved. */

#include "modbus_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"
#include "settings_manager.h"
#include "modbus_rtu_slave.h"


#define MODBUS_REGISTER_CF_ID_IDX        ((uint16_t)0)
#define MODBUS_REGISTER_CARDS_IDX        ((uint16_t)MODBUS_REGISTER_CF_ID_IDX + sizeof(settings.cf_id))
#define MODBUS_REGISTER_CARDS_VALUES_IDX ((uint16_t)MODBUS_REGISTER_CARDS_IDX + sizeof(settings.cards))
#define MODBUS_REGISTER_LOG_ID_IDX       ((uint16_t)MODBUS_REGISTER_CARDS_VALUES_IDX + sizeof(settings.cards_values))


typedef struct _modbus_manager_state_t {
	uint8_t      data[MODBUS_REGISTER_SIZE + 10];
    uint8_t      length;
	util_timer_t wait_timer;
	bool         request_in_progress;
	bool         has_new_data;
} modbus_manager_state_t;


void _modbus_manager_response_data_handler(uint8_t* data, uint16_t len);
void _modbus_manager_request_error_handler();
void _modbus_manager_reset();
void _modbus_manager_send_data();
bool _modbus_manager_has_new_data();
void _modbus_manager_check_registers();


const char* MODBUS_TAG = "MDBS";

modbus_manager_state_t modbus_manager_state = {
	.request_in_progress = false,
	.wait_timer          = { 0 },
};


void modbus_manager_init()
{
	modbus_slave_set_slave_id(GENERAL_MODBUS_SLAVE_ID);
	modbus_slave_set_response_data_handler(&_modbus_manager_response_data_handler);
	modbus_slave_set_internal_error_handler(&_modbus_manager_request_error_handler);
}

void modbus_manager_proccess()
{
	if (modbus_manager_state.request_in_progress && !util_is_timer_wait(&modbus_manager_state.wait_timer)) {
		modbus_slave_timeout();
		_modbus_manager_reset();
		return;
	}

	if (modbus_manager_state.length) {
		_modbus_manager_send_data();
		_modbus_manager_reset();
		return;
	}

	if (_modbus_manager_has_new_data()) {
		_modbus_manager_check_registers();
	}
}

void modbus_manager_recieve_data_byte(uint8_t byte)
{
	modbus_slave_recieve_data_byte(byte);
	util_timer_start(&modbus_manager_state.wait_timer, GENERAL_BUS_TIMEOUT_MS);
}

bool _modbus_manager_has_new_data()
{
	bool tmp_state = modbus_manager_state.has_new_data;
	modbus_manager_state.has_new_data = false;
	return tmp_state;
}

void _modbus_manager_check_registers()
{
	settings_t tmp_settings = { 0 };
	for (uint16_t i = 0; i < sizeof(tmp_settings.cf_id); i++) {
		tmp_settings.cf_id <<= 8;
		tmp_settings.cf_id |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CF_ID_IDX + i);
	}
	for (uint16_t i = 0; i < __arr_len(tmp_settings.cards); i++) {
		for (uint16_t j = 0; j < sizeof(*tmp_settings.cards); j++) {
			tmp_settings.cards[i] <<= 8;
			tmp_settings.cards[i] |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CARDS_IDX + i + j);
		}
	}
	for (uint16_t i = 0; i < __arr_len(tmp_settings.cards_values); i++) {
		for (uint16_t j = 0; j < sizeof(*tmp_settings.cards_values); j++) {
			tmp_settings.cards_values[i] <<= 8;
			tmp_settings.cards_values[i] |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CARDS_VALUES_IDX + i + j);
		}
	}
	for (uint16_t i = 0; i < sizeof(tmp_settings.log_id); i++) {
		tmp_settings.log_id <<= 8;
		tmp_settings.log_id |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_LOG_ID_IDX + i);
	}

	settings_status_t status = SETTINGS_OK;
	if (memcmp((uint8_t*)&tmp_settings, (uint8_t*)&settings, sizeof(tmp_settings))) {
		memcpy((uint8_t*)&settings, (uint8_t*)&tmp_settings, sizeof(tmp_settings));
		status = settings_save();
	}

	if (status != SETTINGS_OK) {
		LOG_TAG_BEDUG(MODBUS_TAG, "settings save error=%02x", status);
	}

	// TODO: find and write next log (by settings.log_id) to input registers
}

void _modbus_manager_response_data_handler(uint8_t* data, uint16_t len)
{
	if (!len || len > sizeof(modbus_manager_state.data)) {
		_modbus_manager_reset();
		return;
	}

	modbus_manager_state.length = len;
	for (uint8_t i = 0; i < len; i++) {
		modbus_manager_state.data[i] = data[i];
	}
	modbus_manager_state.has_new_data = true;
}

void _modbus_manager_request_error_handler()
{
	// TODO: do something
}

void _modbus_manager_reset()
{
	bool tmp_state = modbus_manager_state.has_new_data;
	memset((uint8_t*)&modbus_manager_state, 0, sizeof(modbus_manager_state));
	modbus_manager_state.has_new_data = tmp_state;
}

void _modbus_manager_send_data()
{
	HAL_UART_Transmit(&MODBUS_UART, modbus_manager_state.data, modbus_manager_state.length, GENERAL_BUS_TIMEOUT_MS);
}
