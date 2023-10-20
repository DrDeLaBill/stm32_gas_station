#include "UIManager.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "SettingsDB/SettingsDB.h"

#include "main.h"
#include "pump_manager.h"
#include "access_manager.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"


extern SettingsDB settings;


void UIManager::UIProccess()
{
	if (!settings.isLoaded()) {
		indicate_set_load_page();
	} else {
		indicate_set_wait_page();
	}

	if (general_check_errors()) {
		indicate_set_error_page();
	}

	keyboard4x3_proccess();

	access_proccess();

	UIManager::gasProccess();

	indicate_proccess();

	indicate_display();
}

void UIManager::gasProccess()
{
//	if (!device_info.access_granted) {
//		return;
//	}

	if (pump_has_error()) {
		return;
	}

	if (UIManager::checkKeyboardStop()) {
		pump_stop();
		return;
	}

	if (pump_is_working()) {
		return;
	}

	if (!UIManager::checkKeyboardStart()) {
		return;
	}

	uint32_t user_liters       = (uint32_t)atoi((char*)keyboard4x3_get_buffer());
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT * 10);
	}

	uint32_t user_ml = user_liters * liters_multiplier;

	if (user_liters > GENERAL_SESSION_ML_MIN) {
		pump_set_fuel_ml(user_ml);
	}
}

bool UIManager::checkKeyboardStop()
{
	return strnstr((char*)keyboard4x3_get_buffer(), "*", KEYBOARD4X3_BUFFER_SIZE);
}

bool UIManager::checkKeyboardStart()
{
	return strnstr((char*)keyboard4x3_get_buffer(), "#", KEYBOARD4X3_BUFFER_SIZE);
}
