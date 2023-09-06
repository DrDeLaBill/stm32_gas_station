/* Copyright © 2023 Georgy E. All rights reserved. */

#include "record_manager.h"

#include <string.h>
#include <stdbool.h>

#if defined(STM32F100xB) || \
    defined(STM32F100xE) || \
    defined(STM32F101x6) || \
    defined(STM32F101xB) || \
    defined(STM32F101xE) || \
    defined(STM32F101xG) || \
    defined(STM32F102x6) || \
    defined(STM32F102xB) || \
    defined(STM32F103x6) || \
    defined(STM32F103xB) || \
    defined(STM32F103xE) || \
    defined(STM32F103xG) || \
    defined(STM32F105xC) || \
    defined(STM32F107xC)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F405xx) || \
	defined(STM32F415xx) || \
	defined(STM32F407xx) || \
	defined(STM32F417xx) || \
	defined(STM32F427xx) || \
	defined(STM32F437xx) || \
	defined(STM32F429xx) || \
    defined(STM32F439xx) || \
    defined(STM32F401xC) || \
    defined(STM32F401xE) || \
    defined(STM32F410Tx) || \
    defined(STM32F410Cx) || \
    defined(STM32F410Rx) || \
    defined(STM32F411xE) || \
    defined(STM32F446xx) || \
    defined(STM32F469xx) || \
    defined(STM32F479xx) || \
    defined(STM32F412Cx) || \
    defined(STM32F412Zx) || \
    defined(STM32F412Rx) || \
    defined(STM32F412Vx) || \
    defined(STM32F413xx) || \
    defined(STM32F423xx)
    #include "stm32f4xx_hal.h"
#else
	#error "Please select first the target STM32Fxxx device used in your application (in record_manager.c file)"
#endif

#include "main.h"
#include "utils.h"
#include "settings_manager.h"
#include "storage_data_manager.h"
#include "eeprom_at24cm01_storage.h"


#define RECORD_CLUSTERS_MIN    10
#define RECORD_ERROR_COUNT_MAX 10


const char* RECORD_TAG = "RCRD";;


log_record_t log_record = {0};
log_ids_cache_t log_ids_cache = {
    .is_need_to_scan   = true,
    .cur_scan_address  = 0,
    .ids_cache         = {{0}},
    .first_record_addr = 0xFFFFFFFF
};


record_status_t _record_get_next_cache_record_address(uint32_t* addr, uint16_t* record_num);
record_status_t _record_get_next_cache_available_address(uint32_t* addr, uint16_t* record_num);
record_status_t _record_find_cache_address_by_id(uint32_t* addr, uint16_t* record_num, uint32_t id);
void            _record_get_next_cache_id(uint32_t* new_id);
record_status_t _record_cache_scan_storage_records();
bool            _record_storage_is_empty_page_status(storage_status_t status);


record_status_t next_record_load() {
    uint32_t needed_addr       = 0;
    uint16_t needed_record_num = 0;

    if (log_ids_cache.is_need_to_scan) {
        return RECORD_ERROR;
    }

    record_status_t record_status = _record_get_next_cache_record_address(&needed_addr, &needed_record_num);
    if (record_status == RECORD_NO_LOG) {
        return RECORD_NO_LOG;
    }

#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "reccord load: begin\n");
#endif

    if (record_status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord load: error get available address by cache\n");
#endif
        return record_status;
    }

    if (!needed_addr) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord load: needed address - error=%i, needed_addr=%lu\n", record_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));
    storage_status_t storage_status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (storage_status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "next reccord load: storage load - storage_error=%i\n", storage_status);
#endif
        return RECORD_ERROR;
    }

    if (buff.record_magic != RECORDS_CLUST_MAGIC) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "next reccord load: records count - clust has %u records, but needed %u\n", buff.record_magic, RECORDS_CLUST_SIZE);
#endif
        return RECORD_ERROR;
    }

    if (buff.records[needed_record_num].id <= settings.log_id) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "next reccord load: error record id - needed > %lu, recieved %lu\n", settings.log_id, (long unsigned int)RECORDS_CLUST_SIZE);
#endif
        return RECORD_NO_LOG;
    }

    memcpy((uint8_t*)&log_record, (uint8_t*)&buff.records[needed_record_num], sizeof(log_record));

#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "reccord load: end, loaded from page %lu (address=%lu)\n", needed_addr / STORAGE_PAGE_SIZE, needed_addr);
#endif

    return RECORD_OK;
}


record_status_t record_save() {
#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "reccord save: begin\n");
#endif
    uint32_t needed_addr       = 0;
    uint16_t needed_record_num = 0;

    if (log_ids_cache.is_need_to_scan) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord save: error - cache not loaded\n");
#endif
        return RECORD_ERROR;
    }

    record_status_t record_status = _record_get_next_cache_available_address(&needed_addr, &needed_record_num);
    if (record_status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord save: error get available address by cache\n");
#endif
        return RECORD_ERROR;
    }

    if (!needed_addr) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord save: needed address - error=%i, needed_addr=%lu\n", record_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));
    storage_status_t storage_status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (_record_storage_is_empty_page_status(storage_status)) {
    	storage_status = STORAGE_OK;
    }
    if (storage_status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord save: storage load - storage_error=%i, needed_addr=%lu\n", storage_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

    if (buff.record_magic != RECORDS_CLUST_MAGIC) {
        memset((uint8_t*)&buff, 0, sizeof(buff));
    }

    memcpy((uint8_t*)&buff.records[needed_record_num], (uint8_t*)&log_record, sizeof(log_record_t));
    buff.record_magic = RECORDS_CLUST_MAGIC;

    storage_status = storage_save(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (storage_status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "reccord save: storage save - storage_error=%i, needed_addr=%lu\n", storage_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "reccord save: end, saved on page %lu (address=%lu)\n", needed_addr / STORAGE_PAGE_SIZE, needed_addr);
#endif

    log_ids_cache.ids_cache[needed_addr / STORAGE_PAGE_SIZE][needed_record_num] = log_record.id;

    return RECORD_OK;
}

record_status_t record_get_new_id(uint32_t* new_id)
{
#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "get new id: begin\n");
#endif
    *new_id = RECORD_FIRST_ID;

    if (log_ids_cache.is_need_to_scan) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "get new id: error - cache not loaded\n");
#endif
        return RECORD_ERROR;
    }

    _record_get_next_cache_id(new_id);

    if (*new_id <= settings.log_id) {
        *new_id = settings.log_id + 1;
    }

#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "get new id: end, got max id=%lu\n", *new_id);
#endif

    return RECORD_OK;
}

record_status_t record_delete_record(uint32_t id)
{
#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "delete record: begin\n");
#endif
    if (log_ids_cache.is_need_to_scan) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "delete record: error - cache not loaded\n");
#endif
        return RECORD_ERROR;
    }

    uint32_t needed_addr = 0;
    uint16_t needed_record_num = 0;
    record_status_t record_status = _record_find_cache_address_by_id(&needed_addr, &needed_record_num, id);
    if (record_status != RECORD_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "delete record: find cache address - error=%d\n", record_status);
#endif
        return record_status;
    }

    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0xFF,sizeof(buff));
    storage_status_t storage_status = storage_load(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (storage_status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "delete record: storage not loaded - storage_error=%i, needed_addr=%lu\n", storage_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

    buff.records[needed_record_num].id = 0;

    storage_status = storage_save(needed_addr, (uint8_t*)&buff, sizeof(buff));
    if (storage_status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "delete record: storage save - storage_error=%i, needed_addr=%lu\n", storage_status, needed_addr);
#endif
        return RECORD_ERROR;
    }

#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "delete record: end, deleted on page %lu (address=%lu)\n", needed_addr / STORAGE_PAGE_SIZE, needed_addr);
#endif

    log_ids_cache.ids_cache[needed_addr / STORAGE_PAGE_SIZE][needed_record_num] = 0;

    return RECORD_OK;
}

void record_cache_records_proccess()
{
    if (!log_ids_cache.is_need_to_scan) {
        return;
    }

    record_status_t status = _record_cache_scan_storage_records();
    if (status != RECORD_OK) {
        PRINT_MESSAGE(RECORD_TAG, "scan storage (addr=%lu): fail, error=%02x\n", log_ids_cache.cur_scan_address, status);
        log_ids_cache.is_need_to_scan = true;
    }
}

record_status_t _record_cache_scan_storage_records()
{
    static uint8_t last_scan_percent = 0;
    uint8_t current_scan_percent = ((log_ids_cache.cur_scan_address * 10) / (eeprom_get_size() - STORAGE_PAGE_SIZE)) * 10;
    if (last_scan_percent != current_scan_percent) {
        last_scan_percent = current_scan_percent;
        PRINT_MESSAGE(RECORD_TAG, "Storage scanning proccess: %u%%\n", current_scan_percent);
    }
#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "scan storage (addr=%lu): begin\n", log_ids_cache.cur_scan_address);
#endif
    uint32_t first_addr = 0;
    storage_status_t status = storage_get_first_available_addr(&first_addr);
    if (status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage error (addr=%lu): error get first address - status:=%d, first-address:=%lu\n", log_ids_cache.cur_scan_address, status, first_addr);
#endif
        return RECORD_ERROR;
    }

    uint32_t next_addr = 0;
    if (!log_ids_cache.cur_scan_address) {
        log_ids_cache.cur_scan_address = first_addr;
    }
    status = storage_get_next_available_addr(log_ids_cache.cur_scan_address, &next_addr);
    if (status == STORAGE_ERROR_OUT_OF_MEMORY) {
        goto do_next_addr;
    }
    if (status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage error (addr=%lu): error get next address - status:=%d, next-address:=%lu\n", log_ids_cache.cur_scan_address, status, next_addr);
#endif
        return RECORD_ERROR;
    }

    if (log_ids_cache.cur_scan_address == first_addr) {
        log_ids_cache.first_record_addr = next_addr;
    }

    static uint8_t read_errors_count = 0;
    log_record_clust_t buff;
    memset((uint8_t*)&buff, 0 ,sizeof(buff));
    status = storage_load(next_addr, (uint8_t*)&buff, sizeof(buff));
    if (_record_storage_is_empty_page_status(status)) {
        read_errors_count = 0;
        goto do_next_addr;
    }
    if (status != STORAGE_OK) {
        read_errors_count++;
    }
    if (read_errors_count > RECORD_ERROR_COUNT_MAX) {
        read_errors_count = 0;
        goto do_next_addr;
    }
    if (status != STORAGE_OK) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage error (addr=%lu): read record error=%i, next-address=%lu\n", log_ids_cache.cur_scan_address, status, next_addr);
#endif
        return RECORD_ERROR;
    }

    if (buff.record_magic != RECORDS_CLUST_MAGIC) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage error (addr=%lu): records count - clust has %u records, but needed %u\n", log_ids_cache.cur_scan_address, buff.record_magic, RECORDS_CLUST_SIZE);
#endif
        goto do_next_addr;
    }

    for (uint16_t i = 0; i < RECORDS_CLUST_SIZE; i++) {
        log_ids_cache.ids_cache[next_addr / STORAGE_PAGE_SIZE][i] = buff.records[i].id;
    }

do_next_addr:

#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage (addr=%lu): end - OK\n", log_ids_cache.cur_scan_address);
#endif

    log_ids_cache.cur_scan_address = next_addr;

    if (status == STORAGE_ERROR_OUT_OF_MEMORY) {
#if RECORD_DEBUG
        LOG_TAG_BEDUG(RECORD_TAG, "scan storage: end - OK\n");
#endif
        log_ids_cache.is_need_to_scan = false;
    }

    if (current_scan_percent == 100) {
        PRINT_MESSAGE(RECORD_TAG, "Storage scanning proccess: done\n");
    }

    return RECORD_OK;
}

void _record_get_next_cache_id(uint32_t* new_id)
{
    *new_id = RECORD_FIRST_ID;
#if RECORD_DEBUG
    uint32_t addr = 0;
#endif

    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
            if (log_ids_cache.ids_cache[i][j] > *new_id) {
                *new_id = log_ids_cache.ids_cache[i][j];
#if RECORD_DEBUG
                addr = i * STORAGE_PAGE_SIZE;
#endif
            }
        }
    }
    *new_id += 1;
#if RECORD_DEBUG
    LOG_TAG_BEDUG(RECORD_TAG, "get new id: end, got max id from cache page %lu, new id=%lu\n", addr, *new_id);
#endif
}

record_status_t _record_get_next_cache_record_address(uint32_t* addr, uint16_t* record_num)
{
    if (!log_ids_cache.first_record_addr) {
        return RECORD_ERROR;
    }
    uint32_t last_id = settings.log_id;
    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
            if (log_ids_cache.ids_cache[i][j] <= settings.log_id) {
                continue;
            }
            if (last_id == settings.log_id || last_id > log_ids_cache.ids_cache[i][j]) {
                last_id = log_ids_cache.ids_cache[i][j];
                *addr = i * STORAGE_PAGE_SIZE;
                *record_num = j;
            }
        }
    }
    if (last_id == settings.log_id) {
        return RECORD_NO_LOG;
    }
    return RECORD_OK;
}

record_status_t _record_get_next_cache_available_address(uint32_t* addr, uint16_t* record_num)
{
    if (!log_ids_cache.first_record_addr) {
        return RECORD_ERROR;
    }
    uint32_t min_id = 0xFFFFFFFF;
    uint32_t min_id_addr = 0;
    uint16_t min_id_num = 0;
    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
            if (!log_ids_cache.ids_cache[i][j]) {
                *addr = i * STORAGE_PAGE_SIZE;
                *record_num = j;
                return RECORD_OK;
            }
            if (min_id > log_ids_cache.ids_cache[i][j]) {
                min_id = log_ids_cache.ids_cache[i][j];
                min_id_addr = i * STORAGE_PAGE_SIZE;
                min_id_num = j;
            }
        }
    }
    if (!min_id_addr) {
        return RECORD_ERROR;
    }
    *addr = min_id_addr;
    *record_num = min_id_num;
    return RECORD_OK;
}

record_status_t _record_find_cache_address_by_id(uint32_t* addr, uint16_t* record_num, uint32_t id)
{
    if (!log_ids_cache.first_record_addr) {
        return RECORD_ERROR;
    }
    for (uint32_t i = log_ids_cache.first_record_addr / STORAGE_PAGE_SIZE; i < STORAGE_PAGES_COUNT; i++) {
        for (uint16_t j = 0; j < RECORDS_CLUST_SIZE; j++) {
            if (log_ids_cache.ids_cache[i][j] == id) {
                *addr = i * STORAGE_PAGE_SIZE;
                *record_num = j;
                return RECORD_OK;
            }
        }
    }
    return RECORD_OK;
}

bool _record_storage_is_empty_page_status(storage_status_t status)
{
    return status == STORAGE_ERROR_CRC ||
    	   status == STORAGE_ERROR_BLOCKED ||
           status == STORAGE_ERROR_BADACODE ||
           status == STORAGE_ERROR_VER ||
           status == STORAGE_ERROR_APPOINTMENT ||
		   status == STORAGE_ERROR_EMPTY;
}

