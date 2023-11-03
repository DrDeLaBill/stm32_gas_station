#include "SettingsDB.h"

#include <algorithm>
#include <string.h>

#include "UI.h"
#include "StorageAT.h"

#include "utils.h"


#define EXIT_CODE(_code_) { UI::resetLoad(); return _code_; }


extern StorageAT storage;


const uint8_t SettingsDB::SETTINGS_PREFIX[Page::STORAGE_PAGE_PREFIX_SIZE] = "STG";


SettingsDB::SettingsDB()
{
    memset(reinterpret_cast<void*>(&this->settings), 0, sizeof(this->settings));
    memset(reinterpret_cast<void*>(&this->info), 0, sizeof(this->info));
    this->isSettingsLoaded = false;
    this->info.saved_new_data = true;
}

SettingsDB::SettingsStatus SettingsDB::load()
{
    UI::setLoad();

    uint32_t address = 0;
    StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "error load settings");
#endif
        this->isSettingsLoaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    Settings tmpSettings = { 0 };
    status = storage.load(address, reinterpret_cast<uint8_t*>(&tmpSettings), sizeof(tmpSettings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "error load settings");
#endif
        this->isSettingsLoaded = false;
        EXIT_CODE(SETTINGS_ERROR);
    }

    memcpy(reinterpret_cast<void*>(&this->settings), reinterpret_cast<void*>(&tmpSettings), sizeof(this->settings));

    this->isSettingsLoaded = true;

    settings.cards[0]  = 20056288; //TODO: test
    settings.limits[0] = 100000;   //TODO: test

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "settings loaded");
#endif

    EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::save()
{
    UI::setLoad();

    uint32_t address = 0;
    StorageStatus status = storage.find(FIND_MODE_EQUAL, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    if (status == STORAGE_NOT_FOUND) {
        status = storage.find(FIND_MODE_EMPTY, &address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1);
    }
    if (status == STORAGE_NOT_FOUND) {
        //TODO: save on first page
    }
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "error save settings");
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    status = storage.deleteData(address);
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "error rewrite settings");
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    status = storage.save(address, const_cast<uint8_t*>(SETTINGS_PREFIX), 1, reinterpret_cast<uint8_t*>(&this->settings), sizeof(this->settings));
    if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
        LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "error save settings");
#endif
        EXIT_CODE(SETTINGS_ERROR);
    }

    info.saved_new_data = true;

#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "settings saved");
#endif

    EXIT_CODE(SETTINGS_OK);
}

SettingsDB::SettingsStatus SettingsDB::reset()
{
#if SETTINGS_BEDUG
    LOG_TAG_BEDUG(reinterpret_cast<const char*>(SettingsDB::TAG), "reset settings");
#endif

    settings.cf_id     = SETTINGS_VERSION;
    settings.log_id    = 0;
    settings.device_id = SETTINGS_DEVICE_ID_DEFAULT;

    memset(settings.cards, 0, sizeof(settings.cards));
    memset(settings.limits, 0, sizeof(settings.limits));
//    memset(settings.residues, 0, sizeof(settings.residues));

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

void SettingsDB::set_residue(uint32_t used_litters, uint32_t card)
{
//    unsigned idx = 0;
//    bool idxFound = false;
//    for (unsigned i = 0; i < __arr_len(settings.cards); i++) {
//        if (settings.cards[i] == card) {
//            idx = i;
//            idxFound = true;
//            break;
//        }
//    }
//    if (!idxFound) {
//        return;
//    }
//    if (used_litters > settings.residues[idx]) {
//        settings.residues[idx] = 0;
//    } else {
//        settings.residues[idx] -= used_litters;
//    }
//    this->save();
}
