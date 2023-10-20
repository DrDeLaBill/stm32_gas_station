#ifndef SETTINGS_DB_H
#define SETTINGS_DB_H


#include "main.h"

#include "StoragePage.h"


class SettingsDB
{
public:
	typedef enum _SettingsStatus {
		SETTINGS_OK = 0,
		SETTINGS_ERROR
	} SettingsStatus;

	SettingsDB();
	SettingsStatus load();
	SettingsStatus save();
	SettingsStatus reset();
	bool isLoaded();

	typedef struct __attribute__((packed)) _Settings  {
		uint32_t cf_id;
		uint32_t device_id;
		uint32_t cards       [GENERAL_RFID_CARDS_COUNT];
		uint32_t cards_limits[GENERAL_RFID_CARDS_COUNT];
		uint32_t log_id;
	} Settings;

	Settings settings;

	typedef struct _DeviceInfo {
		bool     settings_loaded;
		bool     access_granted;
		uint32_t user_card;
	} DeviceInfo;

	DeviceInfo info;

private:
	const uint8_t SETTINGS_PREFIX[Page::STORAGE_PAGE_PREFIX_SIZE] = "STG";

	static const uint8_t SETTINGS_VERSION            = ((uint8_t)0x01);
	static const uint8_t SETTINGS_DEVICE_ID_SIZE     = ((uint8_t)16);
	static const uint32_t SETTINGS_DEVICE_ID_DEFAULT = ((uint32_t)1);

	bool isSettingsLoaded;
};


#endif
