#include "RecordDB.h"

#include <stdint.h>
#include <string.h>

#include "StorageAT.h"
#include "SettingsDB.h"

#include "utils.h"
#include "indicate_manager.h"


#define EXIT_CODE(_code_) { indicate_set_wait_page(); return _code_; }


extern StorageAT storage;

extern SettingsDB settings;


const uint8_t RecordDB::RECORD_PREFIX[Page::STORAGE_PAGE_PREFIX_SIZE] = "RCR";


RecordDB::RecordDB(uint32_t recordId)
{
	this->record.id = recordId;
}

RecordDB::RecordStatus RecordDB::load()
{
	indicate_set_load_page();

	uint32_t address = 0;

	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(RECORD_PREFIX), this->record.id);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error load record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	status = storage.load(address, reinterpret_cast<uint8_t*>(&this->record), sizeof(this->record));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error load record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "record loaded");
#endif

	EXIT_CODE(RECORD_OK);
}

RecordDB::RecordStatus RecordDB::loadNext()
{
	indicate_set_load_page();

	uint32_t address = 0;

	StorageStatus status = storage.find(FIND_MODE_NEXT, &address, const_cast<uint8_t*>(RECORD_PREFIX), this->record.id);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error load next record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	status = storage.load(address, reinterpret_cast<uint8_t*>(&this->record), sizeof(this->record));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error load next record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "next record loaded");
#endif

	EXIT_CODE(RECORD_OK);
}

RecordDB::RecordStatus RecordDB::save()
{
	if (this->record.card == 0 || this->record.used_liters == 0) {
		EXIT_CODE(RECORD_ERROR);
	}

	indicate_set_load_page();

	uint32_t address = 0;

	StorageStatus status = storage.find(FIND_MODE_EMPTY, &address);
	if (status == STORAGE_NOT_FOUND) {
		status = storage.find(FIND_MODE_MIN, &address, const_cast<uint8_t*>(RECORD_PREFIX));
	}
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error save record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	status = storage.save(
		address,
		const_cast<uint8_t*>(RECORD_PREFIX),
		this->record.id,
		reinterpret_cast<uint8_t*>(&this->record),
		sizeof(this->record)
	);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error save record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	settings.info.saved_new_data = true;

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "record saved");
#endif

	EXIT_CODE(RECORD_OK);
}

RecordDB::RecordStatus RecordDB::deleteRecord()
{
	indicate_set_load_page();

	uint32_t address = 0;

	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(RECORD_PREFIX), this->record.id);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error delete record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	storage.deleteData(address);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error delete record");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	settings.info.saved_new_data = true;

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "record deleted");
#endif

	EXIT_CODE(RECORD_OK);
}

RecordDB::RecordStatus RecordDB::getNewId(uint32_t *newId)
{
	indicate_set_load_page();

	uint32_t address = 0;

	StorageStatus status = storage.find(FIND_MODE_MAX, &address, const_cast<uint8_t*>(RECORD_PREFIX));
	if (status == STORAGE_NOT_FOUND) {
		*newId = 1;
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "new ID received");
#endif
		EXIT_CODE(RECORD_OK);
	}
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error get new id");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	RecordDB record;
	status = storage.load(address, reinterpret_cast<uint8_t*>(&record.record), sizeof(record.record));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "error get new id");
#endif
		EXIT_CODE(RECORD_ERROR);
	}

	*newId = record.record.id + 1;

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(reinterpret_cast<const char*>(RecordDB::TAG), "new ID received");
#endif

	EXIT_CODE(RECORD_OK);
}
