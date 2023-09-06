/* Copyright © 2023 Georgy E. All rights reserved. */

#include <storage_data_manager.h>

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "utils.h"
#include "eeprom_at24cm01_storage.h"


const char* STORAGE_TAG = "STRG";

storage_info_t storage_info = {
    .is_errors_list_recieved = false,
    .is_errors_list_updated  = false,
    .errors_addr_end         = 0
};


storage_status_t _storage_write(uint32_t page_addr, uint8_t* buff, uint32_t len, storage_header_status_appointment_t appointment);
storage_status_t _storage_read(uint32_t page_addr, uint8_t* buff, uint32_t len, storage_header_status_appointment_t appointment);

storage_status_t _storage_check_errors_list_pages();
storage_status_t _storage_set_page_blocked(uint32_t page_addr, bool blocked);
storage_status_t _storage_is_page_blocked(uint32_t page_addr, bool* result);

uint32_t _storage_get_size();
uint32_t _storage_get_errors_list_pages_count();
uint32_t _storage_get_errors_list_idx_by_address(uint32_t page_addr);
uint32_t _storage_get_errors_list_num_by_address(uint32_t page_addr);
uint8_t  _storage_get_errors_list_page_bit(uint32_t page_addr);

bool _storage_check_header_status_flag(storage_page_record_t* payload, uint8_t flag);


storage_status_t storage_load(uint32_t addr, uint8_t* data, uint32_t len)
{
#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "storage load: begin");
#endif

    storage_status_t status = _storage_check_errors_list_pages();
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage load: unable to get errors list page");
#endif
        return status;
    }

    bool blocked = true;
    status = _storage_is_page_blocked(addr, &blocked);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage load: check page blocked error=%02X", status);
#endif
        return status;
    }

    if (blocked) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage load: unavailable address - errors list page");
#endif
        return STORAGE_ERROR_BLOCKED;
    }

    status = _storage_read(addr, data, len, STORAGE_HEADER_STATUS_APPOINTMENT_COMMON);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage load: unable to get available addresses");
#endif
        return status;
    }

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "storage load: OK");
#endif
    return STORAGE_OK;
}

storage_status_t storage_save(uint32_t addr, uint8_t* data, uint32_t len)
{
#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "storage save: begin");
#endif

    storage_status_t status = _storage_check_errors_list_pages();
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage save: unable to get errors list page");
#endif
        return status;
    }

    bool blocked = true;
    status = _storage_is_page_blocked(addr, &blocked);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage save: check page blocked error=%02X", status);
#endif
        return status;
    }

    if (blocked) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage save: storage page blocked");
#endif
        return STORAGE_ERROR_BLOCKED;
    }

    status = _storage_write(addr, data, len, STORAGE_HEADER_STATUS_APPOINTMENT_COMMON);
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "storage save: storage write error");
#endif
        return status;
    }

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "storage save: OK");
#endif
    return STORAGE_OK;
}

storage_status_t storage_get_first_available_addr(uint32_t* addr)
{
#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "get first available address");
#endif
    return storage_get_next_available_addr(STORAGE_START_ADDR, addr);
}

storage_status_t storage_get_next_available_addr(uint32_t prev_addr, uint32_t* next_addr)
{
#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: begin", prev_addr);
#endif

    if (prev_addr + sizeof(storage_page_record_t) - 1 > _storage_get_size()) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: error - out of memory", prev_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    storage_status_t status = _storage_check_errors_list_pages();
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: error read errors list page - try to write errors list page", prev_addr);
#endif
        status = storage_reset_errors_list_page();
    }

    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: unable to read errors list page", prev_addr);
#endif
        return STORAGE_ERROR;
    }

    uint32_t start_addr = storage_info.errors_addr_end;
    if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: unable to find storage start address", prev_addr);
#endif
        return STORAGE_ERROR;
    }

    if (prev_addr < start_addr) {
        prev_addr = start_addr;
    }

    uint32_t cur_addr = prev_addr + STORAGE_PAGE_SIZE;
    status = STORAGE_OK;
    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        bool blocked = true;
        status = _storage_is_page_blocked(cur_addr, &blocked);
        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: check page blocked error=%02X", prev_addr, status);
#endif
            return status;
        }

        storage_page_record_t payload = { 0 };
        memset((uint8_t*)&payload, 0xFF, sizeof(payload));
        status = _storage_read(cur_addr, (uint8_t*)&payload.payload, sizeof(payload.payload), STORAGE_HEADER_STATUS_APPOINTMENT_COMMON);
        if (status == STORAGE_ERROR ||
            status == STORAGE_ERROR_BUSY
        ) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: check page struct error=%02X", prev_addr, status);
#endif
            return status;
        }

        eeprom_status_t eeprom_status = eeprom_read(cur_addr, (uint8_t*)&payload, sizeof(payload));
        if (eeprom_status != EEPROM_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: check payload FLASH error=%02X", prev_addr, eeprom_status);
#endif
            return STORAGE_ERROR;
        }

        if (status == STORAGE_OK && !_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_END)) {
            goto do_next_address;
        }

        if (!blocked) {
            *next_addr = cur_addr;
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: next address found  %lu", prev_addr, *next_addr);
#endif
            return STORAGE_OK;
        }

do_next_address:
        cur_addr += STORAGE_PAGE_SIZE;
    }

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "get next of %lu address: there is no available next address, error=%02X", prev_addr, status);
#endif
    return status;
}

storage_status_t storage_reset_errors_list_page() {
#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "WARNING! Trying to reset errors list page");
    LOG_TAG_BEDUG(STORAGE_TAG, "reset errors list pages: begin");
#endif

    storage_status_t status           = STORAGE_OK;
    uint32_t page_addr                = STORAGE_START_ADDR;

    storage_errors_list_page_t buff   = { 0 };
    uint32_t errors_list_num          = 1;
    uint32_t needed_errors_list_pages = _storage_get_errors_list_pages_count();

    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        memset((uint8_t*)&buff, 0, sizeof(buff));
        buff.errors_list_page_num = errors_list_num;

        status = _storage_write(page_addr, (uint8_t*)&buff, sizeof(buff), STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE);

        if (status == STORAGE_ERROR_BUSY) {
            return status;
        }

        page_addr += STORAGE_PAGE_SIZE;

        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "reset errors list pages: write page error=%02X", status);
#endif
            continue;
        }

        if (errors_list_num >= needed_errors_list_pages) {
            break;
        }

        errors_list_num++;
    }

    if (status == STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "reset errors list pages: OK");
#endif
        storage_info.errors_addr_end = page_addr - STORAGE_PAGE_SIZE;
    } else {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "reset errors list pages: error=%02X", status);
#endif
    }

    return status;
}

storage_status_t _storage_write(uint32_t page_addr, uint8_t* buff, uint32_t len, storage_header_status_appointment_t appointment)
{
    storage_page_record_t payload;

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): begin", page_addr);
#endif
    
    uint32_t cur_addr       = page_addr;
    uint32_t cur_len        = 0;
    storage_status_t status = STORAGE_OK;
    bool record_start       = true;

    while (cur_len < len) {
        bool blocked = true;
        status = _storage_is_page_blocked(cur_addr, &blocked);
        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): check page blocked error=%02X", cur_addr, status);
#endif
            return status;
        }

        if (blocked) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): unavailable address - errors list page, go to next page", cur_addr);
#endif
            goto do_next_page;
        }
        
        memset((uint8_t*)&payload, 0xFF, sizeof(payload));

        if (cur_addr + sizeof(payload) - 1 > _storage_get_size()) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): unavailable page address", cur_addr);
#endif
            return STORAGE_ERROR_OUT_OF_MEMORY;
        }

        payload.header.magic   = STORAGE_PAYLOAD_MAGIC;
        payload.header.version = STORAGE_PAYLOAD_VERSION;
        payload.header.status  = appointment;

        uint32_t next_addr = 0;
        status = STORAGE_OK;
        if (_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_APPOINTMENT_COMMON)) {
            status = storage_get_next_available_addr(cur_addr, &next_addr);
        }
        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): unable to get next payload address", cur_addr);
#endif
            return STORAGE_ERROR;
        }

        if (record_start) {
            payload.header.status |= STORAGE_HEADER_STATUS_START;
            record_start = false;
        }

        if (cur_len + sizeof(payload.payload) >= len) {
            payload.header.status |= STORAGE_HEADER_STATUS_END;
        }

        payload.header.next_addr = next_addr;
        if (cur_len + sizeof(payload.payload) >= len) {
            payload.header.next_addr = 0;
        }

        uint32_t cpy_len = sizeof(payload.payload);
        if (cur_len + sizeof(payload.payload) > len) {
            cpy_len = len % sizeof(payload.payload);
        }
        memcpy(payload.payload, buff + cur_len, cpy_len);

        payload.crc = util_get_crc16((uint8_t*)&payload, sizeof(payload) - sizeof(payload.crc));

        status = STORAGE_OK;
        bool errors_list_updated = false;

        eeprom_status_t eeprom_status = eeprom_write(cur_addr, (uint8_t*)&payload, sizeof(payload));
        if (_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_APPOINTMENT_COMMON) && (eeprom_status == EEPROM_ERROR)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): try to write errors list page", cur_addr);
#endif
            status = _storage_set_page_blocked(cur_addr, true);
            errors_list_updated = true;
        }

        if (_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_APPOINTMENT_COMMON) && (status != STORAGE_OK)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): try to reset errors list pages", cur_addr);
#endif
            status = storage_reset_errors_list_page();
        }

        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): update errors list pages error", cur_addr);
#endif
            return status;
        }

        if (errors_list_updated) {
            cur_addr = page_addr;
            cur_len  = 0;
            continue;
        }

        if (eeprom_status == EEPROM_ERROR_BUSY) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): flash write error - flash busy", cur_addr);
#endif
            return STORAGE_ERROR_BUSY;
        }

        if (eeprom_status == EEPROM_ERROR_OOM) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): flash write error - out of memory", cur_addr);
#endif
            return STORAGE_ERROR_OUT_OF_MEMORY;
        }

        if (eeprom_status != EEPROM_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): flash write error", cur_addr);
#endif
            return STORAGE_ERROR;
        }

        storage_page_record_t cmpr_payload = { 0 };
        eeprom_status = eeprom_read(cur_addr, (uint8_t*)&cmpr_payload, sizeof(cmpr_payload));

        if (eeprom_status != EEPROM_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): check record error=%02X", cur_addr, status);
#endif
            return STORAGE_ERROR;
        }

        if (memcmp((uint8_t*)&payload, (uint8_t*)&cmpr_payload, sizeof(payload))) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): compare record error", cur_addr);
#endif
            return STORAGE_ERROR;
        }

#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "write page (address=%lu): OK", cur_addr);
#endif

do_next_page:
        cur_len += sizeof(payload.payload);
        cur_addr = next_addr;
    }

    return status;
}

storage_status_t _storage_read(uint32_t page_addr, uint8_t* buff, uint32_t len, storage_header_status_appointment_t appointment)
{
    storage_page_record_t payload   = { 0 };
    storage_page_record_t cmpr_buff = { 0 };
    memset((uint8_t*)&payload, 0xFF, sizeof(payload));
    memset((uint8_t*)&cmpr_buff, 0xFF, sizeof(cmpr_buff));

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): begin", page_addr);
#endif

    uint32_t cur_addr     = page_addr;
    uint32_t cur_len      = 0;
    eeprom_status_t status = EEPROM_OK;
    bool search_start     = true;

    while (cur_len < len) {
        memset((uint8_t*)&payload, 0xFF, sizeof(payload));

        if (cur_addr + sizeof(payload) - 1 > _storage_get_size()) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): unavailable page address", cur_addr);
#endif
            return STORAGE_ERROR_OUT_OF_MEMORY;
        }

        status = eeprom_read(cur_addr, (uint8_t*)&payload, sizeof(payload));
        if (status == EEPROM_ERROR_BUSY) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): flash read error - flash busy", cur_addr);
#endif
            return STORAGE_ERROR_BUSY;
        }

        if (status == EEPROM_ERROR_OOM) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): flash read error - out of memory", cur_addr);
#endif
            return STORAGE_ERROR_OUT_OF_MEMORY;
        }

        if (status != EEPROM_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): flash read error", cur_addr);
#endif
            return STORAGE_ERROR;
        }

        if (!memcmp((uint8_t*)&payload, (uint8_t*)&cmpr_buff, sizeof(payload))) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): error - empty page", cur_addr);
#endif
            return STORAGE_ERROR_EMPTY;
        }

        if (payload.header.magic != STORAGE_PAYLOAD_MAGIC) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): bad storage magic %08lX!=%08lX", cur_addr, payload.header.magic, STORAGE_PAYLOAD_MAGIC);
#endif
            return STORAGE_ERROR_BADACODE;
        }

        if (payload.header.version != STORAGE_PAYLOAD_VERSION) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): bad storage version %02X!=%02X", cur_addr, payload.header.version, STORAGE_PAYLOAD_VERSION);
#endif
            return STORAGE_ERROR_VER;
        }

        uint16_t crc = util_get_crc16((uint8_t*)&payload, sizeof(payload) - sizeof(payload.crc));
        if (crc != payload.crc) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): wrong storage crc %04x!=%04x", cur_addr, payload.crc, crc);
#endif
            return STORAGE_ERROR_CRC;
        }

        if (!_storage_check_header_status_flag(&payload, appointment)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): wrong storage appointment %i!=%i", cur_addr, payload.header.status & 0xF0, appointment);
#endif
            return STORAGE_ERROR_APPOINTMENT;
        }

        if (search_start && !_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_START)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): this is not start address", cur_addr);
#endif
            return STORAGE_ERROR_ADDR;
        }

        memcpy(buff + cur_len, payload.payload, sizeof(payload.payload));
        cur_len += sizeof(payload.payload);

        if (!payload.header.next_addr && !_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_END)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): next address error", cur_addr);
#endif
            return STORAGE_ERROR_ADDR;
        }

        search_start = false;
        cur_addr     = payload.header.next_addr;
    }

    if (!_storage_check_header_status_flag(&payload, STORAGE_HEADER_STATUS_END)) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): buffer is not loaded, bad buffer length=%lu", page_addr, len);
#endif
        return STORAGE_ERROR_ADDR;
    }

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "read page (address=%lu): OK", page_addr);
#endif

    return STORAGE_OK;
}


storage_status_t _storage_check_errors_list_pages()
{
    if (!storage_info.is_errors_list_updated && storage_info.is_errors_list_recieved) {
        return STORAGE_OK;
    }

#if STORAGE_DEBUG
    LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page: begin");
#endif

    storage_status_t status           = STORAGE_OK;
    uint32_t page_addr                = STORAGE_START_ADDR;
    uint32_t needed_errors_list_pages = _storage_get_errors_list_pages_count();

    storage_errors_list_page_t buff = { 0 };

    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        memset((uint8_t*)&buff, 0, sizeof(buff));

        status = _storage_read(page_addr, (uint8_t*)&buff, sizeof(buff), STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE);

        if (status == STORAGE_ERROR_BUSY) {
            return status;
        }

        if (status == STORAGE_ERROR_APPOINTMENT ||
            status == STORAGE_ERROR_BADACODE    ||
            status == STORAGE_ERROR_VER
        ) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page=%lu: check meta error", page_addr);
#endif
            break;
        }

        if (status == STORAGE_ERROR_EMPTY) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page=%lu: there is no errors list", page_addr);
#endif
            return STORAGE_ERROR;
        }

        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page=%lu: read error=%02X", page_addr, status);
#endif
            goto do_next_page;
        }

        if (buff.errors_list_page_num >= needed_errors_list_pages) {
#if STORAGE_DEBUG
            LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page=%lu: found last errors list", page_addr);
#endif
            storage_info.errors_addr_end = page_addr;
            break;
        }

do_next_page:
        page_addr += STORAGE_PAGE_SIZE;
    }

    if (status == STORAGE_ERROR_APPOINTMENT) {
        status = storage_reset_errors_list_page();
    }

    if (status != STORAGE_OK) {
        return STORAGE_ERROR;
    }

    if (buff.errors_list_page_num < needed_errors_list_pages) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page: storage fatal error");
#endif
        return STORAGE_ERROR;
    }

    if (status == STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page: OK");
#endif
        storage_info.is_errors_list_updated  = false;
        storage_info.is_errors_list_recieved = true;
    } else {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "read errors list page: error=%02X", status);
#endif
    }

    return status;
}

storage_status_t _storage_set_page_blocked(uint32_t page_addr, bool blocked)
{
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: begin", page_addr, (int)blocked);
#endif

    storage_info.is_errors_list_updated = true;

    if (page_addr > _storage_get_size() - STORAGE_PAGE_SIZE) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: storage out of memory", page_addr, (int)blocked);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    storage_status_t status         = STORAGE_OK;

    uint32_t errors_list_page_addr  = STORAGE_START_ADDR;
    uint32_t needed_errors_list_num = _storage_get_errors_list_num_by_address(page_addr);
    uint32_t needed_list_page_num   = _storage_get_errors_list_idx_by_address(page_addr);

    storage_errors_list_page_t buff = { 0 };
    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        memset((uint8_t*)&buff, 0, sizeof(buff));

        status = _storage_read(errors_list_page_addr, (uint8_t*)&buff, sizeof(buff), STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE);

        if (status == STORAGE_ERROR_BUSY) {
            return status;
        }

        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: read page error=%02X", page_addr, (int)blocked, status);
#endif
            return status;
        }

        if (buff.errors_list_page_num > needed_errors_list_num) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: there is no this error list page", page_addr, (int)blocked);
#endif
            return STORAGE_ERROR;
        }

        if (buff.errors_list_page_num == needed_errors_list_num) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: error list page found on page=%lu", page_addr, (int)blocked, errors_list_page_addr);
#endif
            break;
        }

        errors_list_page_addr += STORAGE_PAGE_SIZE;
    }

    uint8_t page_bit_shift = _storage_get_errors_list_page_bit(page_addr);
    if (blocked) {
        buff.blocked[needed_list_page_num] |=  (uint8_t)(0x01 << page_bit_shift);
    } else {
        buff.blocked[needed_list_page_num] &= ~(uint8_t)(0x01 << page_bit_shift);
    }

    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        status = _storage_write(errors_list_page_addr, (uint8_t*)&buff, sizeof(buff), STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE);

        if (status == STORAGE_ERROR_BUSY) {
            return status;
        }

        if (status == STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: OK", page_addr, (int)blocked);
#endif
            break;
        }

#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "set page=%lu block=%d: write page error=%02X", page_addr, (int)blocked, status);
#endif
        return STORAGE_ERROR;
    }

    return status;
}

storage_status_t _storage_is_page_blocked(uint32_t page_addr, bool* result)
{
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu block: begin", page_addr);
#endif
    if (page_addr > _storage_get_size() - STORAGE_PAGE_SIZE) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu block: storage out of memory", page_addr);
#endif
        return STORAGE_ERROR_OUT_OF_MEMORY;
    }

    storage_status_t status         = STORAGE_OK;

    uint32_t errors_list_page_addr  = STORAGE_START_ADDR;
    uint32_t needed_errors_list_num = _storage_get_errors_list_num_by_address(page_addr);
    uint32_t needed_list_page_num   = _storage_get_errors_list_idx_by_address(page_addr);

    storage_errors_list_page_t buff = { 0 };
    while (status != STORAGE_ERROR_OUT_OF_MEMORY) {
        memset((uint8_t*)&buff, 0, sizeof(buff));

        status = _storage_read(errors_list_page_addr, (uint8_t*)&buff, sizeof(buff), STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE);

        if (status == STORAGE_ERROR_BUSY) {
            return status;
        }

        if (status != STORAGE_OK) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu block: read page error=%02X", page_addr, status);
#endif
            return status;
        }

        if (buff.errors_list_page_num > needed_errors_list_num) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu block: there is no this error list page", page_addr);
#endif
            return STORAGE_ERROR;
        }

        if (buff.errors_list_page_num == needed_errors_list_num) {
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu block: error list page found on page=%lu", page_addr, errors_list_page_addr);
#endif
            break;
        }

        errors_list_page_addr += STORAGE_PAGE_SIZE;
    }

    if (status == STORAGE_OK) {
        uint8_t page_bit_shift = _storage_get_errors_list_page_bit(page_addr);
        *result = ((uint8_t)buff.blocked[needed_list_page_num] & (uint8_t)(0x01 << page_bit_shift)) > 0;
#if STORAGE_DEBUG
        LOG_TAG_BEDUG(STORAGE_TAG, "check page=%lu: OK, block=%d", page_addr, (int)*result);
#endif
    }

    return status;
}

uint32_t _storage_get_size()
{
    return STORAGE_PAGES_COUNT * STORAGE_PAGE_SIZE;
}

uint32_t _storage_get_errors_list_pages_count()
{
    uint32_t storage_pages_count = STORAGE_PAGES_COUNT;
    uint32_t errors_bytes_count  = (storage_pages_count / 8) + (storage_pages_count % 8 ? 1 : 0);
    uint32_t errors_pages_count  = (errors_bytes_count / STORAGE_PAYLOAD_ERRORS_SIZE) + (errors_bytes_count % STORAGE_PAYLOAD_ERRORS_SIZE ? 1 : 0);
    return errors_pages_count;
}

uint32_t _storage_get_errors_list_idx_by_address(uint32_t page_addr)
{
    uint32_t page_num = page_addr / STORAGE_PAGE_SIZE;
    uint32_t byte_num = (page_num / 8) + (page_num % 8 ? 1 : 0);
    return byte_num % STORAGE_PAYLOAD_ERRORS_SIZE;
}

uint32_t _storage_get_errors_list_num_by_address(uint32_t page_addr)
{
    uint32_t page_num = page_addr / STORAGE_PAGE_SIZE;
    uint32_t byte_num = (page_num / 8) + (page_num % 8 ? 1 : 0);
    return (byte_num / STORAGE_PAYLOAD_ERRORS_SIZE) + 1;
}

uint8_t _storage_get_errors_list_page_bit(uint32_t page_addr)
{
    uint32_t page_num = page_addr / STORAGE_PAGE_SIZE;
    return page_num % 8;
}

bool _storage_check_header_status_flag(storage_page_record_t* payload, uint8_t flag)
{
    return payload->header.status & flag;
}
