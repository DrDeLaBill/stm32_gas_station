/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordTmp.h"

#include <cstring>

#include "log.h"
#include "soul.h"
#include "clock.h"
#include "settings.h"

#include "Record.h"
#include "StorageAT.h"
#include "RecordClust.h"
#include "CodeStopwatch.h"


extern StorageAT storage;


uint32_t RecordTmp::lastAddress = 0;


bool RecordTmp::exists()
{
	uint32_t address = 0;
	return storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1) == STORAGE_OK;
}

RecordStatus RecordTmp::init()
{
	return findAddress();
}

RecordStatus RecordTmp::save(const uint32_t card, const uint32_t lastMl)
{
	utl::CodeStopwatch codeStopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (findAddress() != RECORD_OK) {
		return RECORD_ERROR;
	}

	record_t record = {};
	record.card     = card;
	record.used_mls = lastMl;
	record.time     = clock_get_timestamp();

	StorageStatus storageStatus = storage.rewrite(lastAddress, PREFIX, 1, reinterpret_cast<uint8_t*>(&record), sizeof(record));
    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to save temporary record, error=%u", storageStatus);
#endif
        return RECORD_ERROR;
    }

    return RECORD_OK;
}

RecordStatus RecordTmp::findAddress()
{
	if (lastAddress) {
		return RECORD_OK;
	}

	StorageStatus storageStatus = storage.find(FIND_MODE_EQUAL, &lastAddress, PREFIX, 1);

    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "There is no temporary record in the memory, try to search an empty address");
#endif
        storageStatus = storage.find(FIND_MODE_EMPTY, &lastAddress);
    }

    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "There is no empty address in the memory, try to search a MIN record cluster address");
#endif
        storageStatus = storage.find(FIND_MODE_MIN, &lastAddress, RecordClust::PREFIX);
    }

    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to find an address for temporary record, error=%u", storageStatus);
#endif
        return RECORD_ERROR;
    }

    storageStatus = storage.clearAddress(lastAddress);
    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to clear address, error=%u", storageStatus);
#endif
        return RECORD_ERROR;
    }

    return RECORD_OK;
}

RecordStatus RecordTmp::restore()
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    storageStatus = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1);
    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to find temporary record, error=%u", storageStatus);
#endif
        return RECORD_ERROR;
    }

    record_t recordTmp = {};
    storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&recordTmp), sizeof(recordTmp));
    if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to load temporary record, error=%u", storageStatus);
#endif
        return RECORD_ERROR;
    }

    Record record(0);
    memcpy(reinterpret_cast<uint8_t*>(&record.record), reinterpret_cast<uint8_t*>(&recordTmp), sizeof(recordTmp));
#ifndef POWER_Pin
    record.record.used_mls += TRIG_LEVEL_ML;
#endif

#if RECORD_TMP_BEDUG
	printTagLog(TAG, "Adding %lu used milliliters for %lu card", record.record.used_mls, record.record.card);
#endif

    settings_add_used_liters(record.record.used_mls, record.record.card);
    set_settings_update_status(true);

#if RECORD_TMP_BEDUG
	printTagLog(TAG, "Record is saving");
#endif

	RecordStatus recordStatus = record.save(recordTmp.time);
    if (recordStatus != RECORD_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to save record, error=%u", recordStatus);
#endif
        return recordStatus;
    }

#if RECORD_TMP_BEDUG
	printTagLog(TAG, "Record has been saved");
#endif

	lastAddress = 0;

    return RecordTmp::remove();
}

RecordStatus RecordTmp::remove()
{
    StorageStatus storageStatus = storage.deleteData(PREFIX, 1);
	if (storageStatus != STORAGE_OK) {
#if RECORD_TMP_BEDUG
        printTagLog(TAG, "Unable to clear temporary record, error=%u", storageStatus);
        return RECORD_ERROR;
#endif
	}

	lastAddress = 0;

    return RECORD_OK;
}
