/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


#define SW_VERSION         ((uint8_t)0x03)
#define FW_VERSION         ((uint8_t)0x01)
#define DEFAULT_CF_VERSION ((uint8_t)0x01)
#define DEFAULT_ID         ((uint8_t)0x01)
#define RFID_CARDS_COUNT   ((uint16_t)40)


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


extern settings_t settings;


typedef struct _settings_info_t {
	bool settings_initialized;
	bool settings_saved;
	bool settings_updated;
	bool saved_new_data;
} settings_info_t;


/* copy settings to the target */
settings_t* settings_get();
/* copy settings from the target */
void settings_set(settings_t* other);
/* reset current settings to default */
void settings_reset(settings_t* other);

uint32_t settings_size();

bool settings_check(settings_t* other);

void settings_show();

bool is_settings_saved();
bool is_settings_updated();
bool is_settings_initialized();
bool is_new_data_saved();

void set_settings_initialized();
void set_settings_save_status(bool state);
void set_settings_update_status(bool state);
void set_new_data_saved(bool state);

void settings_set_cf_id(uint32_t cf_id);
void settings_set_device_id(uint32_t device_id);
void settings_set_cards(void* cards, uint16_t len);
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
