/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings_manager.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"
#include "storage_data_manager.h"


const char* SETTINGS_TAG = "STNG";

settings_t settings = {
	.cf_id        = SETTINGS_VERSION,
	.cards        = { 0 },
	.cards_values = { 0 },
	.log_id       = 0
};

deviece_info_t device_info = {
	.settings_loaded = false,
	.access_granted  = false,
	.user_card       = 0,
};


settings_status_t settings_reset()
{
	settings.cf_id  = SETTINGS_VERSION;
	settings.log_id = 0;

	memset(settings.cards, 0, sizeof(settings.cards));
	memset(settings.cards_values, 0, sizeof(settings.cards_values));

	memcpy(settings.device_id, SETTINGS_DEVICE_ID_DEFAULT, __min(strlen(SETTINGS_DEVICE_ID_DEFAULT), sizeof(settings.device_id)));

	return settings_save();
}

settings_status_t settings_load()
{
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "load settings: begin");
#endif

	uint32_t sttngs_addr    = 0;
	storage_status_t status = storage_get_first_available_addr(&sttngs_addr);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "load settings: get settings address=%lu error=%02x", sttngs_addr, status);
#endif
		return SETTINGS_ERROR;
	}

	status = storage_load(sttngs_addr, (uint8_t*)&settings, sizeof(settings));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(SETTINGS_TAG, "load settings: load settings error=%02x", status);
#endif
		return SETTINGS_ERROR;
	}


#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "load settings: OK");
#endif

	device_info.settings_loaded = true;

	return SETTINGS_OK;
}

settings_status_t settings_save()
{
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "save settings: begin");
#endif

	uint32_t sttngs_addr    = 0;
	storage_status_t status = storage_get_first_available_addr(&sttngs_addr);
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "save settings: get settings address=%lu error=%02x", sttngs_addr, status);
#endif
		return SETTINGS_ERROR;
	}

	status = storage_save(sttngs_addr, (uint8_t*)&settings, sizeof(settings));
	if (status != STORAGE_OK) {
#if SETTINGS_BEDUG
		LOG_TAG_BEDUG(SETTINGS_TAG, "save settings: load settings error=%02x", status);
#endif
		return SETTINGS_ERROR;
	}

#if SETTINGS_BEDUG
	LOG_TAG_BEDUG(SETTINGS_TAG, "save settings: OK");
#endif

	return SETTINGS_OK;
}

bool settings_loaded()
{
	return device_info.settings_loaded;
}

void settings_update_cf_id(uint32_t cf_id)
{
	settings.cf_id = cf_id;
}

void settings_update_device_id(uint8_t* device_id, uint16_t len)
{
	memcpy(settings.device_id, device_id, __min(len, sizeof(settings.device_id)));
}

void settings_update_cards(uint32_t* cards, uint16_t len)
{
	memcpy(settings.cards, cards, __min(len, sizeof(settings.cards)));
}

void settings_update_cards_values(uint32_t* cards_values, uint16_t len)
{
	memcpy(settings.cards_values, cards_values, __min(len, sizeof(settings.cards_values)));
}

void settings_update_log_id(uint32_t log_id)
{
	settings.log_id = log_id;
}
