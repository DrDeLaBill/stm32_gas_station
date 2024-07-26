/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define DEVICE_MAJOR (1)
#define DEVICE_MINOR (2)
#define DEVICE_PATCH (3)


/*
 * Device types:
 * 0x0001 - Dispenser
 * 0x0002 - Gas station
 * 0x0003 - Logger
 * 0x0004 - B.O.B.A.
 * 0x0005 - Calibrate station
 * 0x0006 - Dispenser-mini
 */
#define DEVICE_TYPE        ((uint16_t)0x0002) // TODO: add to settings
#define SW_VERSION         ((uint8_t)0x05)
#define FW_VERSION         ((uint8_t)0x01)
#define DEFAULT_CF_VERSION ((uint8_t)0x01)
#define DEFAULT_ID         ((uint8_t)0x01)
#define RFID_CARDS_COUNT   ((uint16_t)51)

#define SETTINGS_MASTER_CARD  (1255648)
#define SETTINGS_MASTER_LIMIT (1000000)


typedef enum _SettingsStatus {
    SETTINGS_OK = 0,
    SETTINGS_ERROR
} SettingsStatus;


typedef enum _LimitType {
	LIMIT_DAY   = 0x01,
	LIMIT_MONTH = 0x02
} LimitType;


typedef struct __attribute__((packed)) _settings_t  {
	// Configuration version
    uint32_t cf_id;
    // Software version
    uint8_t  sw_id;
    // Firmware version
    uint8_t  fw_id;
    uint32_t device_id;
    uint32_t cards      [RFID_CARDS_COUNT];
    uint32_t limits     [RFID_CARDS_COUNT];
    uint8_t  limit_type [RFID_CARDS_COUNT];
    uint32_t used_liters[RFID_CARDS_COUNT];
    uint32_t log_id;
    uint8_t  last_day;
    uint8_t  last_month;
} settings_t;


typedef struct __attribute__((packed)) _settings_v4_t  {
	// Configuration version
    uint32_t cf_id;
    // Software version
    uint8_t  sw_id;
    // Firmware version
    uint8_t  fw_id;
    uint32_t device_id;
    uint32_t cards      [RFID_CARDS_COUNT];
    uint32_t limits     [RFID_CARDS_COUNT];
    uint8_t  limit_type [RFID_CARDS_COUNT];
    uint32_t used_liters[RFID_CARDS_COUNT];
    uint32_t log_id;
    uint8_t  last_day;
    uint8_t  last_month;
} settings_v4_t;


#define RFID_CARDS_COUNT_V3 (40)
typedef struct __attribute__((packed)) _settings_v3_t  {
	// Configuration version
    uint32_t cf_id;
    // Software version
    uint8_t  sw_id;
    // Firmware version
    uint8_t  fw_id;
    uint32_t device_id;
    uint32_t cards      [RFID_CARDS_COUNT_V3];
    uint32_t limits     [RFID_CARDS_COUNT_V3];
    uint8_t  limit_type [RFID_CARDS_COUNT_V3];
    uint32_t used_liters[RFID_CARDS_COUNT_V3];
    uint32_t log_id;
    uint8_t  last_day;
    uint8_t  last_month;
} settings_v3_t;


#define RFID_CARDS_COUNT_V2 (20)
typedef struct __attribute__((packed)) _settings_v2_t  {
	// Configuration version
    uint32_t cf_id;
    // Software version
    uint8_t  sw_id;
    // Firmware version
    uint8_t  fw_id;
    uint32_t device_id;
    uint32_t cards      [RFID_CARDS_COUNT_V2];
    uint32_t limits     [RFID_CARDS_COUNT_V2];
    uint8_t  limit_type [RFID_CARDS_COUNT_V2];
    uint32_t used_liters[RFID_CARDS_COUNT_V2];
    uint32_t log_id;
    uint8_t  last_day;
    uint8_t  last_month;
} settings_v2_t;


extern settings_t settings;


/* copy settings to the target */
settings_t* settings_get();
/* copy settings from the target */
void settings_set(settings_t* other);
/* reset current settings to default */
void settings_reset(settings_t* other);

uint32_t settings_size();

bool settings_check(settings_t* other);
void settings_repair(settings_t* other);

void settings_show();

void settings_set_cf_id(uint32_t cf_id);
void settings_set_device_id(uint32_t device_id);
void settings_set_limits(void* limits, uint16_t len);
void settings_set_log_id(uint32_t log_id);
void settings_set_card(uint32_t card, uint16_t idx);
void settings_set_limit(uint32_t limit, uint16_t idx);
void settings_set_limit_type(LimitType type, uint16_t idx);
void settings_add_used_liters(uint32_t used_litters, uint32_t card);
void settings_clear_limit(uint32_t idx);

SettingsStatus settings_get_card_idx(uint32_t card, uint16_t* idx);
void settings_check_residues();


#ifdef __cplusplus
}
#endif


#endif
