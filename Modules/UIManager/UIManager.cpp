#include "UIManager.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "Access.h"
#include "SettingsDB.h"

#include "main.h"
#include "pump_manager.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"


extern SettingsDB settings;


void UIManager::UIProccess()
{
	if (general_check_errors()) {
		indicate_set_error_page();
	}

	Access::tick();

	if (!Access::isGranted()) {
		indicate_set_wait_page();
		return;
	}

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

	indicate_set_buffer_page();
	indicate_set_buffer(keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);

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
