#include "SettingsDB.h"

#include <algorithm>
#include <string.h>

#include "StorageAT.h"

#include "utils.h"
#include "indicate_manager.h"


#define EXIT_CODE(_code_) { indicate_set_wait_page(); return _code_; }


extern StorageAT storage;


SettingsDB::SettingsDB()
{
	memset(reinterpret_cast<void*>(&this->settings), 0, sizeof(this->settings));
	memset(reinterpret_cast<void*>(&this->info), 0, sizeof(this->info));
	this->isSettingsLoaded = false;
	this->info.saved_new_data = true;
}

SettingsDB::SettingsStatus SettingsDB::load()
{
	indicate_set_load_page();

	uint32_t address = 0;
	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	if (status != STORAGE_OK) {
		this->isSettingsLoaded = false;
		EXIT_CODE(SETTINGS_ERROR);
	}

	Settings tmpSettings = { 0 };
	status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpSettings), sizeof(tmpSettings));
	if (status != STORAGE_OK) {
		this->isSettingsLoaded = false;
		EXIT_CODE(SETTINGS_ERROR);
	}

	memcpy(reinterpret_cast<void*>(&this->settings), reinterpret_cast<void*>(&tmpSettings), sizeof(this->settings));

	this->isSettingsLoaded = true;

	settings.cards[0]  = 20056288; //TODO: test
	settings.limits[0] = 100000;   //TODO: test

	EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::save()
{
	indicate_set_load_page();

	uint32_t address = 0;
	StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	if (status == STORAGE_NOT_FOUND) {
		status = storage.find(FIND_MODE_EMPTY, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
	}
	if (status == STORAGE_NOT_FOUND) {
		//TODO: save on first page
	}
	if (status != STORAGE_OK) {
		EXIT_CODE(SETTINGS_ERROR);
	}

	status = storage.save(address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1, reinterpret_cast<uint8_t*>(&this->settings), sizeof(this->settings));
	if (status != STORAGE_OK) {
		EXIT_CODE(SETTINGS_ERROR);
	}

	info.saved_new_data = true;

	EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::reset()
{
	settings.cf_id     = SETTINGS_VERSION;
	settings.log_id    = 0;
	settings.device_id = SETTINGS_DEVICE_ID_DEFAULT;

	memset(settings.cards, 0, sizeof(settings.cards));
	memset(settings.limits, 0, sizeof(settings.limits));

	return this->save();
}

bool SettingsDB::isLoaded()
{
	return this->isSettingsLoaded;
}

void SettingsDB::set_cf_id(uint32_t cf_id)
{
	if (cf_id) {
		settings.cf_id = cf_id;
	}
}

void SettingsDB::set_device_id(uint32_t device_id)
{
	if (device_id) {
		settings.device_id = device_id;
	}
}

void SettingsDB::set_cards(void* cards, uint16_t len)
{
	if (cards) {
		memcpy(settings.cards, cards, std::min(static_cast<unsigned>(len), sizeof(settings.cards)));
	}
}

void SettingsDB::set_limits(void* limits, uint16_t len)
{
	if (limits) {
		memcpy(settings.limits, limits, std::min(static_cast<unsigned>(len), sizeof(settings.limits)));
	}
}

void SettingsDB::set_log_id(uint32_t log_id)
{
	settings.log_id = log_id;
}

void SettingsDB::set_card(uint32_t card, uint16_t idx)
{
	if (idx < __arr_len(settings.cards)) {
		settings.cards[idx] = card;
	}
}

void SettingsDB::set_limit(uint32_t limit, uint16_t idx)
{
	if (idx < __arr_len(settings.limits)) {
		settings.limits[idx] = limit;
	}
}
