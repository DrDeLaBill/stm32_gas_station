#include "SettingsDB.h"

#include <algorithm>
#include <string.h>

#include "UI.h"
#include "StorageAT.h"

#include "log.h"
#include "utils.h"
#include "clock.h"
#include "settings.h"
#include "eeprom_at24cm01_storage.h"


#define EXIT_CODE(_code_) { UI::resetLoad(); return _code_; }


extern settings_t settings;
extern StorageAT storage;


SettingsDB::SettingsDB(uint8_t* settings, uint32_t size): size(size), settings(settings) { }

SettingsStatus SettingsDB::load()
{
	uint32_t address = 0;
	StorageStatus status = STORAGE_OK;

	bool needResave = false;
    status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: try to find duplicate (error=%02X)", status);
#endif
        status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 2);
        needResave = true;
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: storage find error=%02X", status);
#endif
        return SETTINGS__ERROR;
    }

    uint8_t tmpSettings[this->size] = {}; // TODO: warn
    status = storage.load(address, tmpSettings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error load settings: storage load error=%02X address=%lu", status, address);
#endif
        return SETTINGS__ERROR;
    }

    memcpy(settings, tmpSettings, this->size);

#if SETTINGS_BEDUG
    printTagLog(SettingsDB::TAG, "settings loaded");
#endif

    if (needResave) {
    	set_settings_update_status(true);
    }

    return SETTINGS_OK;
}

SettingsStatus SettingsDB::save()
{
	uint32_t address = 0;
	StorageStatus status = STORAGE_OK;

    status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 1);

    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find error, try to find duplicate (error=%02X)", status);
#endif
    	status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 2);
    }

    if (status == STORAGE_NOT_FOUND) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find duplicate error, try to find empty (error=%02X)", status);
#endif
        status = storage.find(FIND_MODE_EMPTY, &address);
    }

    if (status == STORAGE_NOT_FOUND) {
        // Search for any address
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find empty error, try to find any address (error=%02X)", status);
#endif
    	status = storage.find(FIND_MODE_NEXT, &address, "", 0);
    }

    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage find error=%02X", status);
#endif
        return SETTINGS__ERROR;
    }

    // Save original settings
	status = storage.rewrite(address, PREFIX, 1, this->settings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings: storage save error=%02X address=%lu", status, address);
#endif
        return SETTINGS__ERROR;
    }

    // Save duplicate settings
	status = storage.find(FIND_MODE_EQUAL, &address, PREFIX, 2);

    if (status == STORAGE_NOT_FOUND) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings duplicate: storage find error, try to find empty (error=%02X)", status);
#endif
        status = storage.find(FIND_MODE_EMPTY, &address);
    }

	status = storage.rewrite(address, PREFIX, 2, this->settings, this->size);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        printTagLog(SettingsDB::TAG, "error save settings duplicate: storage save error=%02X address=%lu", status, address);
#endif
        return SETTINGS__ERROR;
    }

    if (this->load() == SETTINGS_OK) {
#if SETTINGS_BEDUG
    	printTagLog(SettingsDB::TAG, "settings saved successfully", address);
#endif

    	return SETTINGS_OK;
    }

    return SETTINGS__ERROR;
}
