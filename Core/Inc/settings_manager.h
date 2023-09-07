/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_MANAGER_H_
#define _SETTINGS_MANAGER_H_


#include <stdint.h>
#include <stdbool.h>

#include "main.h"


#define SETTINGS_BEDUG             (true)
#define SETTINGS_VERSION           ((uint8_t)0x01)
#define SETTINGS_DEVICE_ID_SIZE    ((uint8_t)16)
#define SETTINGS_DEVICE_ID_DEFAULT ("000000000000001")


typedef enum _settings_status_t {
	SETTINGS_OK = 0,
	SETTINGS_ERROR
} settings_status_t;


typedef struct __attribute__((packed)) _settings_t  {
	uint32_t cf_id;
	uint8_t  device_id   [SETTINGS_DEVICE_ID_SIZE];
	uint32_t cards       [GENERAL_RFID_CARDS_COUNT];
	uint32_t cards_values[GENERAL_RFID_CARDS_COUNT];
	uint32_t log_id;
} settings_t;

typedef struct _deviece_info_t {
	bool     settings_loaded;
	bool     access_granted;
	uint32_t user_card;
} deviece_info_t;


extern settings_t      settings;
extern deviece_info_t device_info;

settings_status_t settings_reset();
settings_status_t settings_load();
settings_status_t settings_save();

void settings_update_cf_id(uint32_t cf_id);
void settings_update_device_id(uint8_t* device_id, uint16_t len);
void settings_update_cards(uint32_t* cards, uint16_t len);
void settings_update_cards_values(uint32_t* cards_values, uint16_t len);
void settings_update_log_id(uint32_t log_id);

bool settings_loaded();


#endif
