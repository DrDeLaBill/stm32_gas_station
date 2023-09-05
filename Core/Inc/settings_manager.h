/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_MANAGER_H_
#define _SETTINGS_MANAGER_H_


#include <stdint.h>

#include "main.h"


#define SETTINGS_BEDUG   (true)
#define SETTINGS_VERSION ((uint8_t)0x01)


typedef enum _settings_status_t {
	SETTINGS_OK = 0,
	SETTINGS_ERROR
} settings_status_t;


typedef struct __attribute__((packed)) _settings_t  {
	uint8_t  version;
	uint32_t admin_card;
	uint32_t user_cards[GENERAL_RFID_CARDS_COUNT];
	uint32_t session_liters_max;
} settings_t;


extern settings_t settings;

settings_status_t settings_reset();
settings_status_t settings_load();
settings_status_t settings_save();


#endif
