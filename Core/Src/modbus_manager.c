/* Copyright © 2023 Georgy E. All rights reserved. */

#include "modbus_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f4xx_hal_rtc.h"

#include "main.h"
#include "clock.h"
#include "utils.h"
#include "record_manager.h"
#include "settings_manager.h"
#include "modbus_rtu_slave.h"


#define MODBUS_U8_ARRAY_REG_COUNT(arr)   (sizeof(arr) / sizeof(uint16_t) + sizeof(arr) % sizeof(uint16_t))

#define MODBUS_REGISTER_CF_ID_IDX        ((uint16_t)0)
#define MODBUS_REGISTER_DEVICE_ID_IDX    ((uint16_t)MODBUS_REGISTER_CF_ID_IDX + MODBUS_U8_ARRAY_REG_COUNT(settings.cf_id))
#define MODBUS_REGISTER_CARDS_IDX        ((uint16_t)MODBUS_REGISTER_DEVICE_ID_IDX + MODBUS_U8_ARRAY_REG_COUNT(settings.device_id))
#define MODBUS_REGISTER_CARDS_VALUES_IDX ((uint16_t)MODBUS_REGISTER_CARDS_IDX + MODBUS_U8_ARRAY_REG_COUNT(settings.cards))
#define MODBUS_REGISTER_LOG_ID_IDX       ((uint16_t)MODBUS_REGISTER_CARDS_VALUES_IDX + MODBUS_U8_ARRAY_REG_COUNT(settings.cards_values))
#define MODBUS_REGISTER_YEAR_IDX         ((uint16_t)MODBUS_REGISTER_LOG_ID_IDX + MODBUS_U8_ARRAY_REG_COUNT(settings.log_id))
#define MODBUS_REGISTER_MONTH_IDX        ((uint16_t)MODBUS_REGISTER_YEAR_IDX + MODBUS_U8_ARRAY_REG_COUNT(uint8_t))
#define MODBUS_REGISTER_DAY_IDX          ((uint16_t)MODBUS_REGISTER_MONTH_IDX + MODBUS_U8_ARRAY_REG_COUNT(uint8_t))
#define MODBUS_REGISTER_HOUR_IDX         ((uint16_t)MODBUS_REGISTER_DAY_IDX + MODBUS_U8_ARRAY_REG_COUNT(uint8_t))
#define MODBUS_REGISTER_MINUTE_IDX       ((uint16_t)MODBUS_REGISTER_HOUR_IDX + MODBUS_U8_ARRAY_REG_COUNT(uint8_t))
#define MODBUS_REGISTER_SECOND_IDX       ((uint16_t)MODBUS_REGISTER_MINUTE_IDX + MODBUS_U8_ARRAY_REG_COUNT(uint8_t))

#define MODBUS_REGISTER_LOG_ID_IDX       ((uint16_t)0)
#define MODBUS_REGISTER_TIME_IDX         ((uint16_t)MODBUS_REGISTER_LOG_ID_IDX + MODBUS_U8_ARRAY_REG_COUNT(log_record.log_id))
#define MODBUS_REGISTER_RCRD_CF_ID_IDX   ((uint16_t)MODBUS_REGISTER_TIME_IDX + MODBUS_U8_ARRAY_REG_COUNT(log_record.time))
#define MODBUS_REGISTER_CARD_IDX         ((uint16_t)MODBUS_REGISTER_CF_ID_ID + MODBUS_U8_ARRAY_REG_COUNT(log_record.cf_id))
#define MODBUS_REGISTER_USED_LITERS_IDX  ((uint16_t)MODBUS_REGISTER_CARD_ID + MODBUS_U8_ARRAY_REG_COUNT(log_record.card))


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

void _modbus_manager_write_to_registers();
void _modbus_manager_check_registers();


const char* MODBUS_TAG = "MDBS";

modbus_manager_state_t modbus_manager_state = {
    .data                = { 0 },
    .length              = 0,
    .wait_timer          = { 0 },
    .request_in_progress = false,
    .has_new_data        = false,
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
    } else {
        _modbus_manager_write_to_registers();
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
    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(tmp_settings.cf_id); i++) {
        tmp_settings.cf_id <<= 16;
        tmp_settings.cf_id |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CF_ID_IDX + i);
    }

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(settings.device_id); i++) {
        uint16_t reg_val = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_DEVICE_ID_IDX + i);
        settings.device_id[i] = (uint8_t)(reg_val >> 8);
        if (i + 1 < sizeof(settings.device_id)) {
            settings.device_id[i] = (uint8_t)(reg_val & 0xFF);
        }
    }

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(tmp_settings.cards); i++) {
        for (uint16_t j = 0; j < sizeof(*tmp_settings.cards); j++) {
            tmp_settings.cards[i] <<= 16;
            tmp_settings.cards[i] |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CARDS_IDX + i + j);
        }
    }

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(tmp_settings.cards_values); i++) {
        for (uint16_t j = 0; j < sizeof(*tmp_settings.cards_values); j++) {
            tmp_settings.cards_values[i] <<= 16;
            tmp_settings.cards_values[i] |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_CARDS_VALUES_IDX + i + j);
        }
    }

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(tmp_settings.log_id); i++) {
        tmp_settings.log_id <<= 16;
        tmp_settings.log_id |= modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_LOG_ID_IDX + i);
    }

    settings_update_cf_id(tmp_settings.cf_id);
    settings_update_device_id(tmp_settings.device_id, __arr_len(settings.device_id));
    settings_update_cards(tmp_settings.cards, __arr_len(settings.cards));
    settings_update_cards_values(tmp_settings.cards_values, __arr_len(settings.cards_values));
    settings_update_log_id(tmp_settings.log_id);

    settings_status_t status = SETTINGS_OK;
    if (memcmp((uint8_t*)&tmp_settings, (uint8_t*)&settings, sizeof(tmp_settings))) {
        status = settings_save();
    }

    if (status != SETTINGS_OK) {
        LOG_TAG_BEDUG(MODBUS_TAG, "settings save error=%02x", status);
    }


    RTC_DateTypeDef date = {
        .Year  = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_YEAR_IDX),
        .Month = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_MONTH_IDX),
        .Date  = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_DAY_IDX),
    };
    clock_save_date(&date);

    RTC_TimeTypeDef time = {
        .Hours   = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_HOUR_IDX),
        .Minutes = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_MINUTE_IDX),
        .Seconds = modbus_slave_get_register_value(MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS, MODBUS_REGISTER_SECOND_IDX),
    };
    clock_save_time(&time);

    modbus_manager_state.has_new_data = false;
}

void _modbus_manager_write_to_registers()
{
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_CF_ID_IDX,
        settings.cf_id >> 16
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_CF_ID_IDX + 1,
        settings.cf_id
    );

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(settings.device_id); i++) {
        uint16_t device_reg = ((uint16_t)settings.device_id[2 * i]) << 8;
        if (2 * i + 1 < sizeof(settings.device_id)) {
            device_reg += ((uint16_t)settings.device_id[2 * i + 1]);
        }
        modbus_slave_set_register_value(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            MODBUS_REGISTER_DEVICE_ID_IDX + i,
            device_reg
        );
    }

    for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
        modbus_slave_get_register_value(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            MODBUS_REGISTER_CARDS_IDX + 2 * i,
            settings.cards[i] >> 16
        );
        modbus_slave_get_register_value(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            MODBUS_REGISTER_CARDS_IDX + 2 * i + 1,
            settings.cards[i]
        );
    }

    for (uint16_t i = 0; i < __arr_len(settings.cards_values); i++) {
        modbus_slave_get_register_value(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            MODBUS_REGISTER_CARDS_IDX + 2 * i,
            settings.cards_values[i] >> 16
        );
        modbus_slave_get_register_value(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            MODBUS_REGISTER_CARDS_IDX + 2 * i + 1,
            settings.cards_values[i]
        );
    }

    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_LOG_ID_IDX,
        settings.log_id >> 16
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_LOG_ID_IDX + 1,
        settings.log_id
    );


    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_YEAR_IDX,
        clock_get_year()
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_MONTH_IDX,
        clock_get_month()
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_DAY_IDX,
        clock_get_date()
    );


    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_HOUR_IDX,
        clock_get_hour()
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_MINUTE_IDX,
        clock_get_minute()
    );
    modbus_slave_set_register_value(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        MODBUS_REGISTER_SECOND_IDX,
        clock_get_second()
    );


    log_record_t tmp_record;
    memset((uint8_t*)&tmp_record, 0, sizeof(tmp_record));

    record_status_t record_status = next_record_load();
    if (record_status == RECORD_OK) {
    	memcpy((uint8_t*)&tmp_record, (uint8_t*)&log_record, sizeof(tmp_record));
    }


    modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_LOG_ID_IDX,
		tmp_record.log_id >> 16
	);
    modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_LOG_ID_IDX + 1,
		tmp_record.log_id
	);

    for (uint16_t i = 0; i < MODBUS_U8_ARRAY_REG_COUNT(tmp_record.time); i++) {
    	uint16_t reg_val = ((uint16_t)tmp_record.time[2 * i]) << 16;
        if (2 * i + 1 < sizeof(mp_record.time)) {
        	reg_val += ((uint16_t)mp_record.time[2 * i + 1]);
        }
        modbus_slave_set_register_value(
    		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
    		MODBUS_REGISTER_TIME_IDX + i,
			reg_val
    	);
    }


    modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_RCRD_CF_ID_IDX,
		tmp_record.cf_id >> 16
	);
	modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_RCRD_CF_ID_IDX + 1,
		tmp_record.cf_id
	);


	modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_CARD_IDX,
		tmp_record.card >> 16
	);
	modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_CARD_IDX + 1,
		tmp_record.card
	);


	modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_USED_LITERS_IDX,
		tmp_record.used_liters >> 16
	);
	modbus_slave_set_register_value(
		MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
		MODBUS_REGISTER_USED_LITERS_IDX + 1,
		tmp_record.used_liters
	);
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
