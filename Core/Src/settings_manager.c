/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings_manager.h"

#include <string.h>

#include "main.h"
#include "utils.h"
#include "storage_data_manager.h"


const char* SETTINGS_TAG = "STNG";

settings_t settings = {
	.version    = SETTINGS_VERSION,
	.admin_card = 0,
	.user_cards = { 0 }
};


settings_status_t settings_reset()
{
	settings.version    = SETTINGS_VERSION;
	settings.admin_card = 0;

	memset(settings.user_cards, 0, sizeof(settings.user_cards));

	settings.session_liters_max = GENERAL_SESSION_ML_MAX;

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
