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

typedef struct _settings_info_t {
	bool settings_loaded;
} settings_info_t;


extern settings_t      settings;
extern settings_info_t settings_info;

settings_status_t settings_reset();
settings_status_t settings_load();
settings_status_t settings_save();

bool settings_loaded();


#endif
