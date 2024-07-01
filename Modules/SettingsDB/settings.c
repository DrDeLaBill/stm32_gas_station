/* Copyright © 2023 Georgy E. All rights reserved. */

#include "settings.h"

#include <string.h>
#include <stdbool.h>

#include "glog.h"
#include "main.h"
#include "soul.h"
#include "gutils.h"
#include "clock.h"
#include "hal_defs.h"


#define IS_LIMIT_TYPE(TYPE) ((TYPE) == LIMIT_DAY || (TYPE) == LIMIT_MONTH)


#ifdef DEBUG
static const char SETTINGS_TAG[] = "STNG";
#endif


settings_t settings = { 0 };


settings_t* settings_get()
{
	return &settings;
}

void settings_set(settings_t* other)
{
	memcpy((uint8_t*)&settings, (uint8_t*)other, sizeof(settings));
	if (!settings_check(&settings)) {
		settings_repair(&settings);
	}
}

void settings_reset(settings_t* other)
{
	printTagLog(SETTINGS_TAG, "Reset settings");

    other->cf_id      = DEFAULT_CF_VERSION;
    other->sw_id      = SW_VERSION;
    other->fw_id      = FW_VERSION;
    other->log_id     = 0;
    other->device_id  = DEFAULT_ID;

    for (unsigned i = 0; i < __arr_len(other->cards); i++) {
    	other->cards[i] = 0;
    	other->limits[i] = 0;
    	other->used_liters[i] = 0;
    	other->limit_type[i] = LIMIT_DAY;
    }

    other->last_day   = (uint8_t)clock_get_date();
    other->last_month = (uint8_t)clock_get_month();

	set_status(NEED_SAVE_SETTINGS);
}

uint32_t settings_size()
{
	return sizeof(settings_t);
}

bool settings_check(settings_t* other)
{
	if (other->sw_id != SW_VERSION) {
		return false;
	}
	if (other->fw_id != FW_VERSION) {
		return false;
	}

	for (unsigned i = 0; i < __arr_len(other->limit_type); i++) {
		LimitType *type = &(other->limit_type[i]);
		if (*type == LIMIT_DAY || *type == LIMIT_MONTH) {
			continue;
		}
		return false;
	}

	return true;
}

void settings_repair(settings_t* other)
{
	set_status(NEED_SAVE_SETTINGS);

	if (other->fw_id != FW_VERSION) {
		other->fw_id = FW_VERSION;
	}

	for (unsigned i = 0; i < __arr_len(other->limit_type); i++) {
		LimitType *type = &(other->limit_type[i]);
		if (IS_LIMIT_TYPE(*type)) {
			continue;
		}
		*type = LIMIT_DAY;
	}

	if (other->sw_id == SW_VERSION) {
		return;
	}

	if (other->sw_id == 0x01) {
		settings_reset(&settings);

		other->sw_id = 0x02;
	}

	if (other->sw_id == 0x02) {
		settings_v2_t settings_v2 = {0};
		memcpy((void*)&settings_v2, (void*)other, sizeof(settings_v2));
		memset(other->cards, 0, sizeof(other->cards));
		memset(other->limits, 0, sizeof(other->limits));
		memset(other->limit_type, 0, sizeof(other->limit_type));
		memset(other->used_liters, 0, sizeof(other->used_liters));
		for (unsigned i = 0; i < __arr_len(settings_v2.cards); i++) {
			other->cards[i] = settings_v2.cards[i];
			other->limits[i] = settings_v2.limits[i];
			other->limit_type[i] = settings_v2.limit_type[i];
			other->used_liters[i] = settings_v2.used_liters[i];
		}

		other->sw_id = 0x03;
	}

	if (other->sw_id == 0x03) {
		settings_v3_t settings_v3 = {0};
		memcpy((void*)&settings_v3, (void*)other, sizeof(settings_v3));
		memset(other->cards, 0, sizeof(other->cards));
		memset(other->limits, 0, sizeof(other->limits));
		memset(other->limit_type, 0, sizeof(other->limit_type));
		memset(other->used_liters, 0, sizeof(other->used_liters));
		for (unsigned i = 0; i < __arr_len(settings_v3.cards); i++) {
			other->cards[i] = settings_v3.cards[i];
			other->limits[i] = settings_v3.limits[i];
			other->limit_type[i] = settings_v3.limit_type[i];
			other->used_liters[i] = settings_v3.used_liters[i];
		}
	}

	if (!settings_check(other)) {
		settings_reset(other);
	}
}

void settings_show()
{
	printPretty("################SETTINGS################\n");
    RTC_DateTypeDef date;
    RTC_TimeTypeDef time;
    if (!clock_get_rtc_date(&date)) {
        memset((void*)(&date), 0, sizeof(date));
    }
    if (!clock_get_rtc_time(&time)) {
        memset((void*)(&time), 0, sizeof(time));
    }
	printPretty("Current time: 20%02u-%02u-%02uT%02u:%02u:%02u\n", date.Year, date.Month, date.Date, time.Hours, time.Minutes, time.Seconds);
	printPretty("Software v%u\n", settings.sw_id);
	printPretty("Firmware v%u\n", settings.fw_id);
	printPretty("Configuration ID: %lu\n", settings.cf_id);
	printPretty("Device ID: %lu\n", settings.device_id);
	printPretty("Last log ID: %lu\n", settings.log_id);
	printPretty("Last day: %u (current=%u)\n", settings.last_day, date.Date);
	printPretty("Last month: %u (current=%u)\n", settings.last_month, date.Month);
	printPretty("CARD       LIMIT      USEDLTR    TYPE\n");
	for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
		printPretty(
			"%010lu %010lu %010lu %s\n",
			settings.cards[i],
			settings.limits[i],
			settings.used_liters[i],
			(settings.limit_type[i] == LIMIT_DAY ? "DAY" : (settings.limit_type[i] == LIMIT_MONTH ? "MONTH" : "UNKNOWN"))
		);
	}
	printPretty("################SETTINGS################\n");
}

SettingsStatus settings_get_card_idx(uint32_t card, uint16_t* idx)
{
	if (!card) {
		return SETTINGS_ERROR;
	}
	for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
		if (settings.cards[i] == card) {
			*idx = i;
			return SETTINGS_OK;
		}
	}
	return SETTINGS_ERROR;
}

void settings_check_residues()
{
	bool settingsChanged = false;
	uint8_t date  = clock_get_date();
	uint8_t month = clock_get_month();
	for (unsigned i = 0; i < __arr_len(settings.limit_type); i++) {
		if ((settings.limit_type[i] == LIMIT_DAY && settings.last_day != date) ||
			(settings.limit_type[i] == LIMIT_MONTH && settings.last_month != month)
		) {
			settings.used_liters[i] = 0;
			settingsChanged = true;
		}
	}
	if (settingsChanged) {
		settings.last_day   = date;
		settings.last_month = month;
	    set_status(NEED_SAVE_SETTINGS);
	}
}

void settings_set_cf_id(uint32_t cf_id)
{
    if (settings.cf_id != cf_id) {
        settings.cf_id = cf_id;
	    set_status(NEED_SAVE_SETTINGS);
    }
}

void settings_set_device_id(uint32_t device_id)
{
    if (settings.device_id != device_id) {
        settings.device_id = device_id;
	    set_status(NEED_SAVE_SETTINGS);
    }
}

void settings_set_cards(void* cards, uint16_t len)
{
    if (len > __arr_len(settings.cards)) {
        return;
    }
    if (memcmp(settings.cards, cards, __min(len, sizeof(settings.cards)))) {
        return;
    }
    if (cards) {
        memcpy(settings.cards, cards, __min(len, sizeof(settings.cards)));
	    set_status(NEED_SAVE_SETTINGS);
    }
}

void settings_set_limits(void* limits, uint16_t len)
{
    if (len > __arr_len(settings.limits)) {
        return;
    }
    if (memcmp(settings.limits, limits, __min(len, sizeof(settings.limits)))) {
        return;
    }
    if (limits) {
        memcpy(settings.limits, limits, __min(len, sizeof(settings.limits)));
	    set_status(NEED_SAVE_SETTINGS);
    }
}

void settings_set_log_id(uint32_t log_id)
{
	if (settings.log_id == log_id) {
		return;
	}
    settings.log_id = log_id;
    set_status(NEED_SAVE_SETTINGS);
}

void settings_set_card(uint32_t card, uint16_t idx)
{
    if (idx >= __arr_len(settings.cards)) {
        return;
    }
    if (settings.cards[idx] == card) {
        return;
    }
    settings.cards[idx] = card;
    set_status(NEED_SAVE_SETTINGS);
}

void settings_set_limit(uint32_t limit, uint16_t idx)
{
    if (idx >= __arr_len(settings.limits)) {
        return;
    }
    if (settings.limits[idx] == limit) {
        return;
    }
    settings.limits[idx] = limit;
    set_status(NEED_SAVE_SETTINGS);
}

void settings_set_limit_type(LimitType type, uint16_t idx)
{
	if (idx >= __arr_len(settings.limit_type)) {
		return;
	}
	if (settings.limit_type[idx] == type) {
		return;
	}
	if (IS_LIMIT_TYPE(type)) {
		settings.limit_type[idx] = type;
	    set_status(NEED_SAVE_SETTINGS);
	}
}

void settings_add_used_liters(uint32_t used_litters, uint32_t card)
{
	if (!used_litters) {
		return;
	}
    uint16_t idx = 0;
    if (card == SETTINGS_MASTER_CARD) {
    	return;
    }
    if (settings_get_card_idx(card, &idx) != SETTINGS_OK) {
    	return;
    }
    settings.used_liters[idx] += used_litters;
    set_status(NEED_SAVE_SETTINGS);
}

void settings_clear_limit(uint32_t idx)
{
	if (idx >= __arr_len(settings.used_liters)) {
		return;
	}
	if (!settings.used_liters[idx]) {
		return;
	}
	settings.used_liters[idx] = 0;
    set_status(NEED_SAVE_SETTINGS);
}
