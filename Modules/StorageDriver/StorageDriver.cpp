/* Copyright © 2023 Georgy E. All rights reserved. */

#include "StorageDriver.h"

#include "glog.h"
#include "bmacro.h"
#include "eeprom_at24cm01_storage.h"


bool StorageDriver::hasBuffer = false;
uint8_t StorageDriver::bufferPage[STORAGE_PAGE_SIZE] = {};
uint32_t StorageDriver::lastAddress = 0;


StorageStatus StorageDriver::read(uint32_t address, uint8_t *data, uint32_t len) {
	eeprom_status_t status = EEPROM_OK;
	if (hasBuffer && lastAddress == address && len == STORAGE_PAGE_SIZE) {
		memcpy(data, bufferPage, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Copy %lu address start", address);
#endif
	} else {
		status = eeprom_read(address, data, len);
#if STORAGE_DRIVER_BEDUG
		printTagLog(TAG, "Read %lu address start", address);
#endif
	}
	BEDUG_ASSERT((status != EEPROM_ERROR_BUSY), "Storage is busy");
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
    if (lastAddress != address && len == STORAGE_PAGE_SIZE) {
    	memcpy(bufferPage, data, STORAGE_PAGE_SIZE);
    	lastAddress = address;
    	hasBuffer = true;
    }
#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Read %lu address success", address);
#endif
    return STORAGE_OK;
}
;

StorageStatus StorageDriver::write(uint32_t address, uint8_t *data, uint32_t len) {
#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address start", address);
#endif
	eeprom_status_t status = eeprom_write(address, data, len);
	hasBuffer = false;
	BEDUG_ASSERT((status != EEPROM_ERROR_BUSY), "Storage is busy");
    if (status == EEPROM_ERROR_BUSY) {
        return STORAGE_BUSY;
    }
    if (status == EEPROM_ERROR_OOM) {
        return STORAGE_OOM;
    }
    if (status != EEPROM_OK) {
        return STORAGE_ERROR;
    }
#if STORAGE_DRIVER_BEDUG
	printTagLog(TAG, "Write %lu address success", address);
#endif
    return STORAGE_OK;
}
