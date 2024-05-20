/* Copyright © 2023 Georgy E. All rights reserved. */

#include <Record.h>
#include "ModbusManager.h"


#include <memory>
#include <string.h>
#include <stdint.h>

#include "log.h"
#include "soul.h"
#include "main.h"
#include "clock.h"
#include "settings.h"
#include "modbus_rtu_slave.h"

#include "UI.h"
#include "ModbusRegister.h"


const char ModbusManager::TAG[] = "MBM";

UART_HandleTypeDef* ModbusManager::huart = nullptr;
uint16_t ModbusManager::data_length = 0;
std::unique_ptr<uint8_t[]> ModbusManager::data;
bool ModbusManager::recievedNewData = false;
bool ModbusManager::requestInProgress = false;
utl::Timer ModbusManager::timer(GENERAL_TIMEOUT_MS);
bool ModbusManager::errorFound = false;
utl::Timer ModbusManager::errorTimer(5000);

#if MB_PROTOCOL_BEDUG
uint16_t ModbusManager::counter = 0;
uint8_t ModbusManager::request[20] = {};
#endif


ModbusManager::ModbusManager(UART_HandleTypeDef* huart): lastHash(0)
{
    modbus_slave_set_slave_id(ModbusManager::SLAVE_ID);
    modbus_slave_set_response_data_handler(&(ModbusManager::response_data_handler));
    modbus_slave_set_internal_error_handler(&(ModbusManager::request_error_handler));

    ModbusManager::huart = huart;
}

void ModbusManager::tick()
{
	if (errorFound && !errorTimer.wait()) {
#if MB_PROTOCOL_BEDUG
        printTagLog(TAG, "Modbus error")
#endif
		modbus_slave_timeout();
		ModbusManager::reset();
		return;
	}

    if (ModbusManager::requestInProgress && !timer.wait()) {
#if MB_PROTOCOL_BEDUG
        printTagLog(TAG, "Modbus timeout")
#endif
        modbus_slave_timeout();
        ModbusManager::reset();
        return;
    }

    if (ModbusManager::data_length) {
        ModbusManager::send_data();
        ModbusManager::reset();
        return;
    }

    if (timer.wait()) {
        return;
    }

    if (is_status(WAIT_LOAD)) {
    	return;
    }

    unsigned settingsHash = util_hash((uint8_t*)&settings, sizeof(settings));
    if (settingsHash != lastHash) {
    	lastHash = settingsHash;
        this->updateData();
    } else if (ModbusManager::recievedNewData) {
        this->loadData();
    }
}

void ModbusManager::recieveByte(uint8_t byte)
{
    modbus_slave_recieve_data_byte(byte);
    ModbusManager::requestInProgress = true;
#if MB_PROTOCOL_BEDUG
    if (counter < sizeof(ModbusManager::request)) {
        ModbusManager::request[ModbusManager::counter++] = byte;
    }
#endif
    timer.start();
}

void ModbusManager::loadData()
{
    ModbusManager::recievedNewData = false;

    settings_t tmpSettings = {};
    memcpy(&tmpSettings, &settings, sizeof(tmpSettings));
    ModbusManager::showLogLine();    printTagLog(TAG, "LOAD FROM MODBUS TABLE");

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load cf_id");
#endif
    std::shared_ptr<ModbusRegister<uint32_t>> reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        0,
        tmpSettings.cf_id
    )->load();
    tmpSettings.cf_id = reg32->get();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load device_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        tmpSettings.device_id
    )->load();
    tmpSettings.device_id = reg32->get();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load cards");
#endif
    for (unsigned i = 0; i < __arr_len(tmpSettings.cards); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            tmpSettings.cards[i]
        )->load();
        tmpSettings.cards[i] = reg32->get();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load limits");
#endif
    for (unsigned i = 0; i < __arr_len(tmpSettings.limits); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            tmpSettings.limits[i]
        )->load();
        tmpSettings.limits[i] = reg32->get();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Load limits types");
#endif
    uint32_t tmpAddress = reg32->getNextAddress();
    std::shared_ptr<ModbusRegister<uint8_t>> reg8;
    for (unsigned i = 0; i < __arr_len(tmpSettings.limit_type); i++) {
        reg8 = ModbusRegister<uint8_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
			tmpAddress,
            tmpSettings.limit_type[i]
        )->load();
        tmpSettings.limit_type[i] = reg8->get();
        tmpAddress = reg8->getNextAddress();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load log_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        tmpSettings.log_id
    )->load();
    tmpSettings.log_id = reg32->get();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Load enable clear limit");
#endif
    uint8_t enableClearLimit = 0;
    reg8 = ModbusRegister<uint8_t>::createRegister(
		MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
		reg32->getNextAddress(),
		enableClearLimit
	)->load();
    enableClearLimit = reg8->get();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Load clear limit index");
#endif
    uint32_t clearLimitIdx = 0;
    reg32 = ModbusRegister<uint32_t>::createRegister(
		MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
		reg8->getNextAddress(),
		clearLimitIdx
	)->load();
    clearLimitIdx = reg32->get();

    if (enableClearLimit && clearLimitIdx < __arr_len(tmpSettings.used_liters)) {
    	tmpSettings.used_liters[clearLimitIdx] = 0;
    }

    if (memcmp(&tmpSettings, settings_get(), sizeof(tmpSettings))) {
    	settings_set(&tmpSettings);
    	set_status(NEED_SAVE_SETTINGS);
    }

    RTC_DateTypeDef date = {};
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load year");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        date.Year
    )->load();
    date.Year = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load month");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        date.Month
    )->load();
    date.Month = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load date");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        date.Date
    )->load();
    date.Date = reg8->get();

    clock_save_date(&date);


    RTC_TimeTypeDef time = {};
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load hours");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        time.Hours
    )->load();
    time.Hours = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load minutes");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        time.Minutes
    )->load();
    time.Minutes = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Load seconds");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        time.Seconds
    )->load();
    time.Seconds = reg8->get();

    clock_save_time(&time);
}

void ModbusManager::updateData()
{
    ModbusManager::showLogLine();    printTagLog(TAG, "UPDATE TO MODBUS TABLE");
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save cf_id");
#endif
    std::shared_ptr<ModbusRegister<uint32_t>> reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        0,
        settings.cf_id
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save device_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        settings.device_id
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save cards");
#endif
    for (unsigned i = 0; i < __arr_len(settings.cards); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            settings.cards[i]
        )->save();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save limits");
#endif
    for (unsigned i = 0; i < __arr_len(settings.limits); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            settings.limits[i]
        )->save();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Save limits types");
#endif
    uint32_t tmpAddress = reg32->getNextAddress();
    std::shared_ptr<ModbusRegister<uint8_t>> reg8;
    for (unsigned i = 0; i < __arr_len(settings.limit_type); i++) {
        reg8 = ModbusRegister<uint8_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
			tmpAddress,
            settings.limit_type[i]
        )->save();
        tmpAddress = reg8->getNextAddress();
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save log_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        settings.log_id
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Save enable clear limit");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
		MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
		reg32->getNextAddress(),
		0
	)->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();
    printTagLog(TAG, "Save clear limit index");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
		MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
		reg8->getNextAddress(),
		0
	);

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save year");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        clock_get_year()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save month");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_month()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save date");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_date()
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save hours");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_hour()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save minutes");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_minute()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save seconds");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_second()
    )->save();


#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save rfid cards count");
#endif
    std::shared_ptr<ModbusRegister<uint16_t>> reg16 = ModbusRegister<uint16_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        0,
        RFID_CARDS_COUNT
    )->save();

    Record record(settings.log_id);
    RecordStatus status = record.loadNext();
    if (status != RECORD_OK) {
        memset(&record.record, 0, sizeof(record.record));
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save record id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg16->getNextAddress(),
        record.record.id
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save record time");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg32->getNextAddress(),
		record.record.time
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save record card");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg32->getNextAddress(),
        record.record.card
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    printTagLog(TAG, "Save record used_liters");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg32->getNextAddress(),
        record.record.used_mls
    )->save();
}

void ModbusManager::response_data_handler(uint8_t* data, uint32_t len)
{
    if (!len) {
        ModbusManager::reset();
        return;
    }

	errorFound = false;

    ModbusManager::data = std::make_unique<uint8_t[]>(len);
    for (unsigned i = 0; i < len; i++) {
        ModbusManager::data[i] = data[i];
    }
    ModbusManager::data_length = static_cast<uint16_t>(len);

    if (!ModbusManager::data) {
    	return;
    }

    if (ModbusManager::isWriteCommand(ModbusManager::data[1])) {
        ModbusManager::recievedNewData = true;
    }
}

void ModbusManager::request_error_handler()
{
	errorFound = true;
	if (!errorFound) {
		errorTimer.start();
	}
}

void ModbusManager::send_data()
{
    if (!ModbusManager::data || !ModbusManager::huart) {
        return;
    }
#if MB_PROTOCOL_BEDUG
    gprint("%s:\trequest  - ", ModbusManager::TAG);
    for (unsigned i = 0; i < ModbusManager::counter; i++) {
        gprint("%02X ", ModbusManager::request[i]);
    }
    gprint("\n");
    gprint("%s:\tresponse - ", ModbusManager::TAG);
    for (unsigned i = 0; i < ModbusManager::data_length; i++) {
    	gprint("%02X ", ModbusManager::data[i]);
    }
    gprint("\n");
#endif
    HAL_UART_Transmit(ModbusManager::huart, ModbusManager::data.get(), ModbusManager::data_length, GENERAL_TIMEOUT_MS);
}

void ModbusManager::reset()
{
	errorFound = false;

    ModbusManager::data = NULL;
    ModbusManager::data_length = 0;
//    memset(reinterpret_cast<void*>(&ModbusManager::timer), 0, sizeof(ModbusManager::timer));

//    ModbusManager::recievedNewData = false;
    ModbusManager::requestInProgress = false;
    ModbusManager::data.reset();

#if MB_PROTOCOL_BEDUG
    ModbusManager::counter = 0;
    memset(ModbusManager::request, 0, sizeof(ModbusManager::request));
#endif
}

bool ModbusManager::isWriteCommand(uint8_t command)
{
    return command == MODBUS_FORCE_SINGLE_COIL ||
		   command == MODBUS_PRESET_SINGLE_REGISTER ||
		   command == MODBUS_FORCE_MULTIPLE_COILS ||
		   command == MODBUS_PRESET_MULTIPLE_REGISTERS;
}

void ModbusManager::showLogLine()
{
	printTagLog(TAG, "------------------------------------------------------------------");
}
