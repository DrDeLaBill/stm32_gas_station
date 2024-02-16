/* Copyright © 2024 Georgy E. All rights reserved. */

#include "RecordClust.h"

#include <cstring>
#include <stdint.h>

#include "log.h"
#include "bmacro.h"
#include "settings.h"

#include "Record.h"
#include "StorageAT.h"
#include "CodeStopwatch.h"


extern StorageAT storage;


RecordClust::RecordClust(uint32_t recordId, uint16_t recordSize):
 	m_recordId(recordId), m_recordSize(recordSize), m_address(0)
{
    memset(reinterpret_cast<void*>(&m_clust), 0, sizeof(m_clust));
}

record_t& RecordClust::operator[](unsigned i)
{
#if RECORD_BEDUG
    BEDUG_ASSERT((i < getCountByRecordSize(m_clust.rcrd_size)), "Cluster records out of range");
#endif
    return m_clust[i];
}

record_t& RecordClust::record_clust_t::operator[](unsigned i)
{
#if RECORD_BEDUG
    BEDUG_ASSERT((i < RecordClust::getCountByRecordSize(rcrd_size)), "Cluster records out of range");
#endif
    return *(reinterpret_cast<record_t*>(&(records[i * rcrd_size])));
}

RecordStatus RecordClust::load(bool validateSize)
{
    bool statusFlag = this->loadExist(validateSize);
    if (statusFlag) {
#if RECORD_BEDUG
        printTagLog(TAG, "Cluster loaded from address=%lu", m_address);
#endif
    } else {
#if RECORD_BEDUG
        printTagLog(TAG, "Unable to load cluster, try to create new");
#endif
        statusFlag = this->createNew();
    }

    if (!statusFlag) {
#if RECORD_BEDUG
        printTagLog(TAG, "Unable to create cluster");
#endif
        return RECORD_ERROR;
    }

    return RECORD_OK;
}

RecordStatus RecordClust::save(record_t *record, uint32_t size)
{
#if RECORD_BEDUG
    printTagLog(TAG, "Saving record (size=%lu bytes)", size);
    BEDUG_ASSERT(size <= RECORDS_SIZE, "Size of record is incorrect");
#endif
    if (size > RECORDS_SIZE) {
        return RECORD_ERROR;
    }

    RecordStatus recordStatus = RECORD_OK;

    // 1. find max id
    uint32_t maxId = 0;
    uint32_t newId = 0;
    recordStatus = this->getMaxId(&maxId); // TODO: assert + update record ID
    if (recordStatus == RECORD_NO_LOG) {
        maxId = 0;
    } else if (recordStatus != RECORD_OK) {
#if RECORD_BEDUG
    	BEDUG_ASSERT(false, "Unable to calculate new ID"); // TODO: assert after 337 line
#endif
        return RECORD_ERROR;
    }
    newId = maxId + 1;

    // 2. nope -----------create record_clust_t variable (tmp)
//    record_clust_t tmpClust = {};

    // 3. load cluster to tmp and validate
    this->m_recordId = maxId;
    recordStatus = this->load(true);
#if RECORD_BEDUG
    BEDUG_ASSERT((recordStatus == RECORD_OK), "Unable to load cluster");
#endif
    if (recordStatus != RECORD_OK) {
        return recordStatus;
    }

    // 4. if sizes has the same values and cluster has empty section, copy current record to tmp
    unsigned emptyIndex = 0;
    {
        bool clustFLag = true;
        bool foundFlag = false;
        for (unsigned i = 0; i < m_clust.count(); i++) {
            if (!(*this)[i].id) {
                emptyIndex = i;
                foundFlag = true;
                break;
            }
        }
        if (!foundFlag) {
            clustFLag = this->createNew();
        }
#if RECORD_BEDUG
        BEDUG_ASSERT(clustFLag, "Unable to create cluster");
#endif
        if (!clustFLag) {
            return RECORD_ERROR;
        }
    }
    record->id = newId;
    memcpy(
        reinterpret_cast<void*>(&(*this)[emptyIndex]),
        reinterpret_cast<void*>(record),
        m_recordSize
    );

    // 5. save cluster
    StorageStatus storageStatus = STORAGE_OK;
    {
        storageStatus = storage.rewrite(m_address, PREFIX, newId, reinterpret_cast<uint8_t*>(&m_clust), m_clust.size());
#if RECORD_BEDUG
        BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage save record error");
#endif
        if (storageStatus != STORAGE_OK) {
            return RECORD_ERROR;
        }

        this->m_recordId = newId;
    }

    // 6. load cluster
    recordStatus = this->load(true);
#if RECORD_BEDUG
    BEDUG_ASSERT((recordStatus == RECORD_OK), "Error loading the saved record");
    if (recordStatus == RECORD_OK) {
        printTagLog(TAG, "Record cluster saved (address=%lu, id=%lu, record_size=%u)", m_address, newId, m_recordSize);
    }
#endif

    return recordStatus;
}

bool RecordClust::validate(record_clust_t* clust)
{
    if (clust->sw_id != settings.sw_id) {
        return false;
    }

    if (!clust->rcrd_size || clust->rcrd_size > RECORDS_SIZE) {
    	return false;
    }

    for (unsigned i = 0; i < getCountByRecordSize(clust->rcrd_size); i++) {
    	if ((*clust)[i].id) {
    		return true;
    	}
    }

    return false;
}

void RecordClust::show()
{
#if RECORD_BEDUG
	printPretty("################RECORD CLUST#################\n");
	printPretty("Software v%02u\n", m_clust.sw_id);
	printPretty("Record size %u\n", m_clust.rcrd_size);
    printPretty("INDEX   RCRDID    TIME     CARD     LITERS\n");
	for (uint8_t i = 0; i < getCountByRecordSize(m_clust.rcrd_size); i++) {
		if (!(*this)[i].id) {
			break;
		}
	    printPretty("%03u     %09lu %08lu %08lu %08lu\n", i, (*this)[i].id, (*this)[i].time, (*this)[i].card, (*this)[i].used_liters);
	}
	if (!getCountByRecordSize(m_clust.rcrd_size)) {
        printPretty("--------------------EMPTY--------------------\n");
	}
	printPretty("################RECORD CLUST#################\n");
#endif
}

bool RecordClust::loadExist(bool validateSize)
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    storageStatus = storage.find(FIND_MODE_EQUAL, &address, PREFIX, m_recordId);

    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(TAG, "Unable to find an EQUAL cluster, the NEXT cluster is being searched");
#endif
        storageStatus = storage.find(FIND_MODE_NEXT, &address, PREFIX, m_recordId);
    }

    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(TAG, "Unable to find an NEXT cluster, the MAX ID cluster is being searched");
#endif
        storageStatus = storage.find(FIND_MODE_MAX, &address, PREFIX);
    }

    if (storageStatus != STORAGE_OK) {
#if RECORD_BEDUG
        printTagLog(TAG, "Unable to find a cluster with error=%u", storageStatus);
#endif
        return false;
    }

    record_clust_t tmpClust = {}; // TODO: load function for structure
	storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), RecordClust::META_SIZE);
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Load record cluster meta error");
#endif
	if (storageStatus != STORAGE_OK) {
		return RECORD_ERROR;
	}
	storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), tmpClust.size());
#if RECORD_BEDUG
	BEDUG_ASSERT((storageStatus == STORAGE_OK), "Load record cluster error");
#endif
	if (storageStatus != STORAGE_OK) {
		return RECORD_ERROR;
	}

    if (!this->validate(&tmpClust)) {
#if RECORD_BEDUG
        printTagLog(TAG, "Validation failed (there is an incorrect cluster in the memory), delete cluster from address=%lu", address);
#endif
        BEDUG_ASSERT(storage.clearAddress(address) == STORAGE_OK, "Delete record error");
        return false;
    }

    if (validateSize && m_recordSize > 0 &&  tmpClust.rcrd_size != m_recordSize) {
#if RECORD_BEDUG
        printTagLog(TAG, "The current cluster has another record size, abort search");
#endif
        return false;
    }

    m_address = address;
    memcpy(
		reinterpret_cast<void*>(&m_clust),
		reinterpret_cast<void*>(&tmpClust),
		tmpClust.size()
    );

    return true;
}

bool RecordClust::createNew()
{
    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;
    StorageFindMode findMode = FIND_MODE_EMPTY;

    storageStatus = storage.find(findMode, &address);
    if (storageStatus == STORAGE_NOT_FOUND || storageStatus == STORAGE_OOM) {
        findMode = FIND_MODE_MIN;
        storageStatus = storage.find(findMode, &address);
    }

#if RECORD_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to find memory for log record");
#endif
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    if (findMode == FIND_MODE_MIN) {
        storageStatus = storage.clearAddress(address);
    }
#if RECORD_BEDUG // TODO: up
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to erase memory for log record");
#endif
    if (storageStatus != STORAGE_OK) {
        return false;
    }

    this->m_clust.sw_id     = settings.sw_id;
    this->m_clust.rcrd_size = m_recordSize;
    memset(this->m_clust.records, 0, sizeof(this->m_clust.records));

    this->m_address = address;

    return true;
}

RecordStatus RecordClust::getMaxId(uint32_t* maxId)
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

    uint32_t address = 0;
    StorageStatus storageStatus = STORAGE_OK;

    storageStatus = storage.find(FIND_MODE_MAX, &address, PREFIX);
    if (storageStatus == STORAGE_NOT_FOUND) {
#if RECORD_BEDUG
        printTagLog(TAG, "MAX ID not found, reset ID");
#endif
        return RECORD_NO_LOG;
    }

#if RECORD_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Storage find error");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    record_clust_t tmpClust = {}; // TODO: load function for structure
    storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), RecordClust::META_SIZE);
#if RECORD_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to load record cluster meta");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }
    storageStatus = storage.load(address, reinterpret_cast<uint8_t*>(&tmpClust), tmpClust.size());
#if RECORD_BEDUG
    BEDUG_ASSERT((storageStatus == STORAGE_OK), "Unable to load record cluster");
#endif
    if (storageStatus != STORAGE_OK) {
        return RECORD_ERROR;
    }

    if (!this->validate(&tmpClust)) {
#if RECORD_BEDUG
        printTagLog(TAG, "Validation failed (there is an incorrect cluster in the memory), delete cluster from address=%lu", address);
#endif
        BEDUG_ASSERT(storage.clearAddress(address) == STORAGE_OK, "Delete record error");
        return RECORD_ERROR;
    }

    *maxId = 0;
    for (unsigned i = 0; i < getCountByRecordSize(tmpClust.rcrd_size); i++) {
    	uint32_t tmpId = tmpClust[i].id;
        if (*maxId < tmpId) {
            *maxId = tmpId;
        }
    }

#if RECORD_BEDUG
    printTagLog(TAG, "new ID received from address=%lu id=%lu", address, *maxId);
#endif

    return RECORD_OK;
}

uint32_t RecordClust::getCountByRecordSize(uint32_t recordSize)
{
#if RECORD_BEDUG
    BEDUG_ASSERT((recordSize > 0), "Record size must be large than 0");
#endif
    if (recordSize == 0) {
        return 0;
    }

    uint32_t payload_clust = Page::PAYLOAD_SIZE - RecordClust::META_SIZE;
    return (recordSize > payload_clust) ? 1 : (payload_clust / recordSize);
}

uint32_t RecordClust::record_clust_t::count()
{
    return getCountByRecordSize(this->rcrd_size);
}

uint32_t RecordClust::record_clust_t::size()
{
#if RECORD_BEDUG
    BEDUG_ASSERT((this->rcrd_size > 0), "Record size must be large than 0");
#endif
	if (!this->count()) {
		return 0;
	}

    return RecordClust::META_SIZE + this->rcrd_size * this->count();
}

uint32_t RecordClust::records_count()
{
	return this->m_clust.count();
}

uint32_t RecordClust::structure_size()
{
	return this->m_clust.size();
}
