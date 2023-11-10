#include "SettingsDB.h"

#include <algorithm>
#include <string.h>

#include "UI.h"
#include "StorageAT.h"

#include "utils.h"
#include "clock.h"


#define EXIT_CODE(_code_) { UI::resetLoad(); return _code_; }


extern StorageAT storage;


const uint8_t SettingsDB::SETTINGS_PREFIX[Page::STORAGE_PAGE_PREFIX_SIZE] = "STG";
const char SettingsDB::TAG[] = "STG";


SettingsDB::SettingsDB()
{
    memset(reinterpret_cast<void*>(&(this->settings)), 0, sizeof(this->settings));
    memset(reinterpret_cast<void*>(&(this->info)), 0, sizeof(this->info));
    this->info.settings_loaded = false;
    this->info.saved_new_data = true;
}

SettingsDB::SettingsStatus SettingsDB::load()
{
    UI::setLoad();

    uint32_t address = 0;
    StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error load settings");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    Settings tmpSettings = { 0 };
    status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpSettings), sizeof(tmpSettings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error load settings");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    if (tmpSettings.cf_id != SETTINGS_VERSION) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error settings version");
#endif
        this->info.settings_loaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    memcpy(reinterpret_cast<void*>(&(this->settings)), reinterpret_cast<void*>(&tmpSettings), sizeof(this->settings));

    this->info.settings_loaded = true;

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "settings loaded");
    this->show();
#endif

    EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::save()
{
    UI::setLoad();

    uint32_t address = 0;
    StorageFindMode mode = FIND_MODE_EQUAL;
    StorageStatus status = storage.find(mode, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    if (status == STORAGE_NOT_FOUND) {
    	mode = FIND_MODE_EMPTY;
        status = storage.find(mode, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    }
    if (status == STORAGE_NOT_FOUND) {
        //TODO: save on first page
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error find settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    if (mode != FIND_MODE_EMPTY) {
    	status = storage.deleteData(address);
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error delete settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    status = storage.save(address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1, reinterpret_cast<uint8_t*>(&(this->settings)), sizeof(this->settings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(SettingsDB::TAG, "error save settings (address=%lu)", address);
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    info.saved_new_data = true;
    info.settings_loaded = false;

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "settings saved (address=%lu)", address);
    this->show();
#endif

    EXIT_CODE(this->load());
}

SettingsDB::SettingsStatus SettingsDB::reset()
{
#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(SettingsDB::TAG, "reset settings");
#endif

    settings.cf_id      = SETTINGS_VERSION;
    settings.log_id     = 0;
    settings.device_id  = SETTINGS_DEVICE_ID_DEFAULT;

    memset(settings.cards, 0, sizeof(settings.cards));
    memset(settings.limits, 0, sizeof(settings.limits));
    memset(settings.limit_type, LIMIT_DAY, sizeof(settings.limit_type));
    memset(settings.used_liters, 0, sizeof(settings.used_liters));

    settings.last_day   = clock_get_date();
    settings.last_month = clock_get_month();

    return this->save();
}

bool SettingsDB::isLoaded()
{
    return this->info.settings_loaded;
}

SettingsDB::SettingsStatus SettingsDB::getCardIdx(uint32_t card, uint16_t* idx)
{
	if (!card) {
		return SETTINGS_ERROR;
	}
	for (unsigned i = 0; i < __arr_len(settings.cards); i++) {
		if (settings.cards[i] == card) {
			*idx = i;
			return SETTINGS_OK;
		}
	}
	return SETTINGS_ERROR;
}

void SettingsDB::checkResidues()
{
	bool settingsChanged = false;
	for (unsigned i = 0; i < __arr_len(settings.limit_type); i++) {
		if ((settings.limit_type[i] == LIMIT_DAY && settings.last_day != clock_get_date()) ||
			(settings.limit_type[i] == LIMIT_MONTH && settings.last_month != clock_get_month())
		) {
			settings.last_day = clock_get_date();
			settings.last_month = clock_get_month();
			settings.used_liters[i] = 0;
			settingsChanged = true;
		}
	}
	if (settingsChanged) {
		this->save();
	}
}

void SettingsDB::show()
{
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SettingsDB::TAG, "------------------------------------------------------------------");
	LOG_TAG_BEDUG(SettingsDB::TAG, "cf_id = %lu", settings.cf_id);
	LOG_TAG_BEDUG(SettingsDB::TAG, "device_id = %lu", settings.device_id);
	for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
		LOG_TAG_BEDUG(SettingsDB::TAG, "CARD %lu: card=%u, limit=%lu, used_liters=%lu, limit_type=%s", i, settings.cards[i], settings.limits[i], settings.used_liters[i], (settings.limit_type[i] == LIMIT_DAY ? "DAY" : (settings.limit_type[i] == LIMIT_MONTH ? "MONTH" : "UNKNOWN")));
	}
	LOG_TAG_BEDUG(SettingsDB::TAG, "log_id = %lu", settings.log_id);
	LOG_TAG_BEDUG(SettingsDB::TAG, "last_day = %u (current=%u)", settings.last_day, clock_get_date());
	LOG_TAG_BEDUG(SettingsDB::TAG, "last_month = %u (current=%u)", settings.last_month, clock_get_month());
	LOG_TAG_BEDUG(SettingsDB::TAG, "------------------------------------------------------------------");
#endif
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
    if (len > __arr_len(settings.cards)) {
        return;
    }
    if (memcmp(settings.cards, cards, std::min(static_cast<unsigned>(len), sizeof(settings.cards)))) {
        return;
    }
    if (cards) {
        memcpy(settings.cards, cards, std::min(static_cast<unsigned>(len), sizeof(settings.cards)));
    }
}

void SettingsDB::set_limits(void* limits, uint16_t len)
{
    if (len > __arr_len(settings.limits)) {
        return;
    }
    if (memcmp(settings.limits, limits, std::min(static_cast<unsigned>(len), sizeof(settings.limits)))) {
        return;
    }
    if (limits) {
        memcpy(settings.limits, limits, std::min(static_cast<unsigned>(len), sizeof(settings.limits)));
    }
}

void SettingsDB::set_log_id(uint32_t log_id)
{
	if (settings.log_id == log_id) {
		return;
	}
    settings.log_id = log_id;
}

void SettingsDB::set_card(uint32_t card, uint16_t idx)
{
    if (idx >= __arr_len(settings.cards)) {
        return;
    }
    if (settings.cards[idx] == card) {
        return;
    }
    settings.cards[idx] = card;
}

void SettingsDB::set_limit(uint32_t limit, uint16_t idx)
{
    if (idx >= __arr_len(settings.limits)) {
        return;
    }
    if (settings.limits[idx] == limit) {
        return;
    }
    settings.limits[idx] = limit;
}

void SettingsDB::set_limit_type(LimitType type, uint16_t idx)
{
	if (idx >= __arr_len(settings.limit_type)) {
		return;
	}
	if (settings.limit_type[idx] == type) {
		return;
	}
	if (type == LIMIT_DAY || type == LIMIT_MONTH) {
		settings.limit_type[idx] = type;
	}
}

void SettingsDB::add_used_liters(uint32_t used_litters, uint32_t card)
{
    uint16_t idx = 0;
    if (this->getCardIdx(card, &idx) != SETTINGS_OK) {
    	return;
    }
    settings.used_liters[idx] += used_litters;
}
