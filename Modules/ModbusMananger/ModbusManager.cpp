/* Copyright © 2023 Georgy E. All rights reserved. */

#include "ModbusManager.h"


#include <memory>
#include <string.h>
#include <stdint.h>

#include "clock.h"
#include "modbus_rtu_slave.h"

#include "UI.h"
#include "RecordDB.h"
#include "SettingsDB.h"
#include "ModbusRegister.h"


const char ModbusManager::TAG[] = "MBM";

UART_HandleTypeDef* ModbusManager::huart = nullptr;
uint16_t ModbusManager::data_length = 0;
std::unique_ptr<uint8_t[]> ModbusManager::data;
util_timer_t ModbusManager::timer;
bool ModbusManager::recievedNewData = false;
bool ModbusManager::requestInProgress = false;

#if MB_MANAGER_BEDUG
uint16_t ModbusManager::counter = 0;
uint8_t ModbusManager::request[20] = {};
#endif


extern SettingsDB settings;


ModbusManager::ModbusManager(UART_HandleTypeDef* huart)
{
    modbus_slave_set_slave_id(ModbusManager::SLAVE_ID);
    modbus_slave_set_response_data_handler(&(ModbusManager::response_data_handler));
    modbus_slave_set_internal_error_handler(&(ModbusManager::request_error_handler));

    ModbusManager::huart = huart;
}

void ModbusManager::tick()
{
    if (ModbusManager::requestInProgress && !util_is_timer_wait(&ModbusManager::timer)) {
#if MB_MANAGER_BEDUG
        LOG_TAG_BEDUG(ModbusManager::TAG, "Modbus timeout")
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

    if (util_is_timer_wait(&ModbusManager::timer)) {
        return;
    }

    if (settings.info.saved_new_data) {
        this->updateData();
    } else if (ModbusManager::recievedNewData) {
        this->loadData();
    }
}

void ModbusManager::recieveByte(uint8_t byte)
{
    modbus_slave_recieve_data_byte(byte);
    ModbusManager::requestInProgress = true;
#if MB_MANAGER_BEDUG
    if (counter < sizeof(ModbusManager::request)) {
        ModbusManager::request[ModbusManager::counter++] = byte;
    }
#endif
    util_timer_start(&ModbusManager::timer, GENERAL_BUS_TIMEOUT_MS);
}

void ModbusManager::loadData()
{
    ModbusManager::recievedNewData = false;

    SettingsDB tmpSettings;
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "LOAD FROM MODBUS TABLE");
#endif

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load cf_id");
#endif
    std::shared_ptr<ModbusRegister<uint32_t>> reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        0,
        tmpSettings.settings.cf_id
    )->load();
    tmpSettings.set_cf_id(reg32->get());

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load device_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        tmpSettings.settings.device_id
    )->load();
    tmpSettings.set_device_id(reg32->get());

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load cards");
#endif
    for (unsigned i = 0; i < __arr_len(tmpSettings.settings.cards); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            tmpSettings.settings.cards[i]
        )->load();
        tmpSettings.set_card(reg32->get(), i);
    }
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load limits");
#endif
    for (unsigned i = 0; i < __arr_len(tmpSettings.settings.cards); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            tmpSettings.settings.limits[i]
        )->load();
        tmpSettings.set_limit(reg32->get(), i);
    }
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load log_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        tmpSettings.settings.log_id
    )->load();
    tmpSettings.set_log_id(reg32->get());

    SettingsDB::SettingsStatus status = SettingsDB::SETTINGS_OK;
    if (memcmp(&tmpSettings.settings, &settings.settings, sizeof(tmpSettings.settings))) {
        memcpy(&settings.settings, &tmpSettings.settings, sizeof(tmpSettings.settings));
        status = settings.save();
    }

    if (status != SettingsDB::SETTINGS_OK) {
#if MB_MANAGER_BEDUG
        LOG_TAG_BEDUG(ModbusManager::TAG, "settings save error=%02x", status);
#endif
    }


    RTC_DateTypeDef date = {};
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load year");
#endif
    std::shared_ptr<ModbusRegister<uint8_t>> reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        date.Year
    )->load();
    date.Year = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load month");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        date.Month
    )->load();
    date.Month = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load date");
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
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load hours");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        time.Hours
    )->load();
    time.Hours = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load minutes");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        time.Minutes
    )->load();
    time.Minutes = reg8->get();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Load seconds");
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
    settings.info.saved_new_data = false;
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "UPDATE TO MODBUS TABLE");
#endif
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save cf_id");
#endif
    std::shared_ptr<ModbusRegister<uint32_t>> reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        0,
        settings.settings.cf_id
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save device_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        settings.settings.device_id
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save cards");
    LOG_TAG_BEDUG(ModbusManager::TAG, "cards count: %d", __arr_len(settings.settings.cards));
#endif
    for (unsigned i = 0; i < __arr_len(settings.settings.cards); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            settings.settings.cards[i]
        )->save();
    }
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save limits");
    LOG_TAG_BEDUG(ModbusManager::TAG, "limits count: %d", __arr_len(settings.settings.limits));
#endif
    for (unsigned i = 0; i < __arr_len(settings.settings.limits); i++) {
        reg32 = ModbusRegister<uint32_t>::createRegister(
            MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
            reg32->getNextAddress(),
            settings.settings.limits[i]
        )->save();
    }
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save log_id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        settings.settings.log_id
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save year");
#endif
    std::shared_ptr<ModbusRegister<uint8_t>> reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg32->getNextAddress(),
        clock_get_year()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save month");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_month()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save date");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_date()
    )->save();

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save hours");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_hour()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save minutes");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_minute()
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save seconds");
#endif
    reg8 = ModbusRegister<uint8_t>::createRegister(
        MODBUS_REGISTER_ANALOG_OUTPUT_HOLDING_REGISTERS,
        reg8->getNextAddress(),
        clock_get_second()
    )->save();


#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save rfid cards count");
#endif
    std::shared_ptr<ModbusRegister<uint16_t>> reg16 = ModbusRegister<uint16_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        0,
        GENERAL_RFID_CARDS_COUNT
    )->save();

    RecordDB record(settings.settings.log_id);
    RecordDB::RecordStatus status = record.loadNext();
    if (status != RecordDB::RECORD_OK) {
        memset(&record.record, 0, sizeof(record.record));
    }

#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save record id");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg16->getNextAddress(),
        record.record.id
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save record time");
#endif
    for (unsigned i = 0, addr = reg32->getNextAddress(); i < __arr_len(record.record.time); i++) {
        reg8 = ModbusRegister<uint8_t>::createRegister(
            MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
            addr,
            record.record.time[i]
        )->save();
        addr = reg8->getNextAddress();
    }
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save record card");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg8->getNextAddress(),
        record.record.card
    )->save();
#if MB_MANAGER_BEDUG
    ModbusManager::showLogLine();    LOG_TAG_BEDUG(ModbusManager::TAG, "Save record used_liters");
#endif
    reg32 = ModbusRegister<uint32_t>::createRegister(
        MODBUS_REGISTER_ANALOG_INPUT_REGISTERS,
        reg32->getNextAddress(),
        record.record.used_liters
    )->save();
}

void ModbusManager::response_data_handler(uint8_t* data, uint32_t len)
{
    if (!len) {
        ModbusManager::reset();
        return;
    }

    ModbusManager::data = std::make_unique<uint8_t[]>(len);
    for (unsigned i = 0; i < len; i++) {
        ModbusManager::data[i] = data[i];
    }
    ModbusManager::data_length = len;

    if (ModbusManager::isWriteCommand(data[1])) {
        ModbusManager::recievedNewData = true;
    }
}

void ModbusManager::request_error_handler()
{
    // TODO: error
}

void ModbusManager::send_data()
{
    if (!ModbusManager::data || !ModbusManager::huart) {
        return;
    }
#if MB_MANAGER_BEDUG
    LOG_BEDUG("%s:\trequest  - ", ModbusManager::TAG);
    for (unsigned i = 0; i < ModbusManager::counter; i++) {
        LOG_BEDUG("%02X ", ModbusManager::request[i]);
    }
    LOG_BEDUG("\n");
    LOG_BEDUG("%s:\tresponse - ", ModbusManager::TAG);
    for (unsigned i = 0; i < ModbusManager::data_length; i++) {
        LOG_BEDUG("%02X ", ModbusManager::data[i]);
    }
    LOG_BEDUG("\n");
#endif
    HAL_UART_Transmit(ModbusManager::huart, ModbusManager::data.get(), ModbusManager::data_length, GENERAL_BUS_TIMEOUT_MS);
}

void ModbusManager::reset()
{
    ModbusManager::data = NULL;
    ModbusManager::data_length = 0;
//    memset(reinterpret_cast<void*>(&ModbusManager::timer), 0, sizeof(ModbusManager::timer));

//    ModbusManager::recievedNewData = false;
    ModbusManager::requestInProgress = false;

#if MB_MANAGER_BEDUG
    ModbusManager::counter = 0;
    memset(ModbusManager::request, 0, sizeof(ModbusManager::request));
#endif
}

bool ModbusManager::isWriteCommand(uint8_t command)
{
    return command > MODBUS_READ_INPUT_REGISTERS;
}

void ModbusManager::showLogLine()
{
#if MB_MANAGER_BEDUG
	LOG_TAG_BEDUG(ModbusManager::TAG, "------------------------------------------------------------------");
#endif
}
