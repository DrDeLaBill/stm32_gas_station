#include "SettingsDB.h"

#include <string.h>

#include "StorageAT.h"


extern StorageAT storage;


SettingsDB::SettingsDB()
{
	this->isSettingsLoaded = false;
	memset(reinterpret_cast<void*>(&this->settings), 0, sizeof(this->settings));
	memset(reinterpret_cast<void*>(&this->info), 0, sizeof(this->info));
}

SettingsDB::SettingsStatus SettingsDB::load()
{
	uint32_t address = 0;
	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	if (status != STORAGE_OK) {
		this->isSettingsLoaded = false;
		return SETTINGS_ERROR;
	}

	Settings tmpSettings = { 0 };
	status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpSettings), sizeof(tmpSettings));
	if (status != STORAGE_OK) {
		this->isSettingsLoaded = false;
		return SETTINGS_ERROR;
	}

	memcpy(reinterpret_cast<void*>(&this->settings), reinterpret_cast<void*>(&tmpSettings), sizeof(this->settings));

	this->isSettingsLoaded = true;

	return SETTINGS_OK;
}

SettingsDB::SettingsStatus SettingsDB::save()
{
	uint32_t address = 0;
	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	if (status == STORAGE_NOT_FOUND) {
		status = storage.find(FIND_MODE_EMPTY, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	}
	if (status != STORAGE_OK) {
		return SETTINGS_ERROR;
	}

	status = storage.save(address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1, reinterpret_cast<uint8_t*>(&this->settings), sizeof(this->settings));
	if (status != STORAGE_OK) {
		return SETTINGS_ERROR;
	}

	return SETTINGS_OK;
}

SettingsDB::SettingsStatus SettingsDB::reset()
{
	settings.cf_id     = SETTINGS_VERSION;
	settings.log_id    = 0;
	settings.device_id = SETTINGS_DEVICE_ID_DEFAULT;

	memset(settings.cards, 0, sizeof(settings.cards));
	memset(settings.cards_limits, 0, sizeof(settings.cards_limits));

	return this->save();
}

bool SettingsDB::isLoaded()
{
	return this->isSettingsLoaded;
}
