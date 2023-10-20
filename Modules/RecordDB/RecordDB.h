#ifndef RECORD_DB_H
#define RECORD_DB_H


#include <stdint.h>

#include "StorageAT.h"


class RecordDB
{
public:
	typedef enum _RecordStatus {
		RECORD_OK = 0,
		RECORD_ERROR,
		RECORD_NO_LOG
	} RecordStatus;

	RecordDB();

	RecordStatus load();
	RecordStatus save();

	static RecordStatus getNewId(uint32_t *newId);
	static RecordStatus deleteRecord(uint32_t targetId);

	static const uint32_t RECORD_TIME_ARRAY_SIZE  = 6;
	typedef struct __attribute__((packed)) _Record {
		uint32_t id;                           // Record ID
		uint8_t  time[RECORD_TIME_ARRAY_SIZE]; // Record time
		uint32_t cf_id;                        // Configuration version
		uint32_t card;                         // User card ID
		uint32_t used_liters;                  // Session used liters
	} Record;

	Record record = { 0 };

private:
	static const uint32_t RECORDS_CLUST_SIZE  = ((Page::STORAGE_PAGE_PAYLOAD_SIZE - sizeof(uint8_t)) / sizeof(struct _Record));
	static const uint32_t RECORDS_CLUST_MAGIC = (sizeof(struct _Record));

	typedef struct __attribute__((packed)) _RecordClust {
		uint8_t record_magic;
		Record  records[RECORDS_CLUST_SIZE];
	} RecordClust;
};


#endif
