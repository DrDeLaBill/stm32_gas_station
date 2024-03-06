/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Record.h"

#include <cstdint>
#include <cstring>

#include "StorageAT.h"
#include "RecordClust.h"
#include "CodeStopwatch.h"

#include "log.h"
#include "soul.h"
#include "utils.h"
#include "clock.h"
#include "settings.h"
#include "eeprom_at24cm01_storage.h"


Record::Record(uint32_t recordId):
	record(), m_recordId(recordId) { }

RecordStatus Record::load()
{
    RecordClust clust(m_recordId, this->size());

    RecordStatus recordStatus = clust.load(true);
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    bool recordFound = false;
    unsigned id;
    for (unsigned i = 0; i < clust.records_count(); i++) {
        if (clust[i].id == this->m_recordId) {
            recordFound = true;
            id = i;
            break;
        }
    }
    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(reinterpret_cast<void*>(&(this->record)), reinterpret_cast<void*>(&(clust[id])), this->size());

#if RECORD_BEDUG
    printTagLog(TAG, "Record loaded (cluster index=%u)", id);
    this->show();
#endif

    return RECORD_OK;
}

RecordStatus Record::loadNext()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

    RecordClust clust(m_recordId + 1, this->size());

    RecordStatus recordStatus = clust.load(false);
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    bool recordFound = false;
    unsigned idx;
    uint32_t curId = 0xFFFFFFFF;
    for (unsigned i = 0; i < clust.records_count(); i++) {
        if (clust[i].id > m_recordId && curId > clust[i].id) {
            curId = clust[i].id;
            recordFound = true;
            idx = i;
            break;
        }
    }

    if (!recordFound) {
#if RECORD_BEDUG
        printTagLog(TAG, "Record not found");
#endif
        return RECORD_NO_LOG;
    }

    memcpy(
        reinterpret_cast<void*>(&(this->record)),
        reinterpret_cast<void*>(&(clust[idx])),
        this->size()
    );

#if RECORD_BEDUG
    printTagLog(TAG, "Next record loaded (cluster index=%u)", idx);
    this->show();
#endif

    return RECORD_OK;
}

RecordStatus Record::save(uint32_t time)
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	set_status(NEED_SAVE_FINAL_RECORD);
	set_status(WAIT_LOAD);

	if (!this->size()) {
		reset_status(NEED_SAVE_FINAL_RECORD);
		reset_status(WAIT_LOAD);
		return RECORD_ERROR;
	}

    RecordClust clust(0, this->size());
    RecordStatus recordStatus = RECORD_OK;

    record.time = time > 0 ? time : clock_get_timestamp();
    recordStatus = clust.save(&record, this->size());

#ifdef RECORD_BEDUG
    if (recordStatus == RECORD_OK) {
    	clust.show();
        this->show();
    } else {
        printTagLog(TAG, "New record was not saved");
    }
#endif

	reset_status(NEED_SAVE_FINAL_RECORD);
	reset_status(WAIT_LOAD);
    return recordStatus;
}

void Record::show()
{
#if RECORD_BEDUG
    printPretty("#########RECORD#########\n");
	printPretty("ID:       %lu\n", record.id);
	printPretty("Time:     %lu\n", record.time);
	printPretty("Card:     %lu\n", record.card);
	printPretty("Used mls: %lu.%03lu l\n", record.used_mls / ML_IN_LTR, record.used_mls % ML_IN_LTR);
    printPretty("#########RECORD#########\n");
#endif
}

void Record::showMax()
{
	RecordClust clust(0, 0);
	clust.showMax();
}

uint16_t Record::size()
{
	return sizeof(record_t);
}
