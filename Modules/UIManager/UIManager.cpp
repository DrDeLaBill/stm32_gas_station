#include "UIManager.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "stm32f4xx_hal.h"

#include "Access.h"
#include "SettingsDB.h"

#include "main.h"
#include "pump_manager.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"


extern SettingsDB settings;


bool UIManager::isStartPressed = false;
char UIManager::constBuffer[KEYBOARD4X3_BUFFER_SIZE] = "";


void UIManager::UIProccess()
{
	Access::tick();

	if (general_check_errors()) {
		indicate_set_error_page();
		return;
	}

	if (!Access::isGranted() && !pump_is_working()) {
		UIManager::clear();
		pump_stop();
		return;
	}

	if (UIManager::checkStop()) {
		UIManager::clear();
		pump_stop();
#if UI_BEDUG
		LOG_TAG_BEDUG(UIManager::TAG, "pump stop command");
#endif
		return;
	}

	if (!pump_is_free()) {
		return;
	}

	indicate_set_buffer_page();
	if (!UIManager::isStartPressed) {
		indicate_set_buffer(keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);
	} else {
		indicate_set_buffer(reinterpret_cast<uint8_t*>(UIManager::constBuffer), KEYBOARD4X3_BUFFER_SIZE);
	}

	// TODO: add timeout

	if (!UIManager::checkStart()) {
		return;
	}

	UIManager::isStartPressed = true;
	memcpy(UIManager::constBuffer, keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);

	uint32_t user_liters       = (uint32_t)atoi(UIManager::constBuffer);
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT * 10);
	}

	uint32_t user_ml = user_liters * liters_multiplier;

	if (user_liters >= GENERAL_SESSION_ML_MIN) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UIManager::TAG, "pump start command");
#endif
		pump_set_fuel_ml(user_ml);
	} else {
#if UI_BEDUG
		LOG_TAG_BEDUG(UIManager::TAG, "invalid liters value");
#endif
		UIManager::clear();
	}
}

bool UIManager::checkKeyboardStop()
{
	return keyboard4x3_is_cancel();
}

bool UIManager::checkKeyboardStart()
{
	return keyboard4x3_is_enter();
}

bool UIManager::checkStop()
{
	return UIManager::checkKeyboardStop() || HAL_GPIO_ReadPin(PUMP_STOP_GPIO_Port, PUMP_STOP_Pin);
}

bool UIManager::checkStart()
{
	return UIManager::checkKeyboardStart() || HAL_GPIO_ReadPin(PUMP_START_GPIO_Port, PUMP_START_Pin);
}

void UIManager::clear()
{
	memset(UIManager::constBuffer, 0, sizeof(UIManager::constBuffer));
	UIManager::isStartPressed = false;
	keyboard4x3_clear();
	indicate_set_wait_page();
}
