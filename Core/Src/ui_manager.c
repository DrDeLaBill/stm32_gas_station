/* Copyright © 2023 Georgy E. All rights reserved. */

#include "ui_manager.h"

#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "pump_manager.h"
#include "access_manager.h"
#include "settings_manager.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"


void _ui_check_user_need_gas();
bool _ui_check_keyboard_stop();
bool _ui_check_keyboard_start();


void ui_proccess()
{
	if (!settings_loaded()) {
		indicate_set_load_page();
	}

	if (general_check_errors()) {
		indicate_set_error_page();
	}

	indicate_proccess();

	keyboard4x3_proccess();

	access_proccess();

	_ui_check_user_need_gas();
}

void _ui_check_user_need_gas()
{
	if (!device_info.access_granted) {
		return;
	}

	if (pump_has_error()) {
		return;
	}

	if (_ui_check_keyboard_stop()) {
		pump_stop();
		return;
	}

	if (pump_is_working()) {
		return;
	}

	if (!_ui_check_keyboard_start()) {
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

bool _ui_check_keyboard_stop()
{
	return strnstr((char*)keyboard4x3_get_buffer(), "*", KEYBOARD4X3_BUFFER_SIZE);
}

bool _ui_check_keyboard_start()
{
	return strnstr((char*)keyboard4x3_get_buffer(), "#", KEYBOARD4X3_BUFFER_SIZE);
}
