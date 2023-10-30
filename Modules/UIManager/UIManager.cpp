#include "UIManager.h"

#include <cmath>
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


std::unique_ptr<UIFSMBase> UIManager::ui = std::make_unique<UIFSMInit>();


void UIManager::UIProccess()
{
	Access::tick();

	if (UIManager::checkErrors()) {
		ui = std::make_unique<UIFSMError>();
	}

	ui->tick();
}

bool UIManager::checkErrors()
{
	return general_check_errors();
}

UIFSMBase::UIFSMBase()
{
	util_timer_start(&timer, getTimerDelayMs());
}

void UIFSMBase::tick()
{
	this->proccess();
	this->checkState();
}

void UIFSMBase::checkState()
{
	if (this->hasErrors) {
		return;
	}

	if (getTimerDelayMs() && !util_is_timer_wait(&timer)) {
		UIManager::ui = std::make_unique<UIFSMStop>();
		pump_stop();
		return;
	}

	if (UIManager::checkKeyboardStop()) {
		UIManager::ui = std::make_unique<UIFSMStop>();
		pump_stop();
		return;
	}

	if (pump_is_working()) {
		UIManager::ui = std::make_unique<UIFSMCount>();
		return;
	}
}

void UIFSMInit::proccess()
{
	UIManager::ui = std::make_unique<UIFSMWait>();
}

void UIFSMWait::proccess()
{
	UIManager::clear();

	if (Access::isGranted()) {
		UIManager::ui = std::make_unique<UIFSMInput>();
	}
//	pump_stop();
}

void UIFSMInput::proccess()
{
	indicate_set_buffer(keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);

	if (UIManager::checkKeyboardStart()) {
		UIManager::ui = std::make_unique<UIFSMStart>();
	}
}

void UIFSMStart::proccess()
{
	uint32_t user_liters       = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}

	uint32_t user_ml = user_liters * liters_multiplier;

	if (user_liters >= GENERAL_SESSION_ML_MIN) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UIManager::TAG, "pump start command");
#endif
		UIManager::ui = std::make_unique<UIFSMCount>();
		pump_set_fuel_ml(user_ml);
		return;
	}

#if UI_BEDUG
	LOG_TAG_BEDUG(UIManager::TAG, "invalid liters value");
#endif
	UIManager::clear();
	UIManager::ui = std::make_unique<UIFSMWait>();
}

void UIFSMCount::proccess()
{
	uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = { 0 };
	uint32_t curr_count = pump_get_fuel_count_ml() / pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	for (unsigned i = util_get_number_len(curr_count); i > 0; i--) {
		buffer[i-1] = '0' + curr_count % 10;
		curr_count /= 10;
	}
	indicate_set_buffer(buffer, KEYBOARD4X3_BUFFER_SIZE);

	if (pump_is_stopped()) {
		UIManager::ui = std::make_unique<UIFSMResult>();
	}
}

void UIFSMStop::proccess()
{
	UIManager::clear();
	pump_stop();

	UIManager::ui = std::make_unique<UIFSMResult>();
}

void UIFSMResult::checkState()
{
	if (UIManager::checkKeyboardStop()) {
		UIManager::ui = std::make_unique<UIFSMWait>();
	}

	if (!util_is_timer_wait(&timer)) {
		UIManager::ui = std::make_unique<UIFSMWait>();
	}
}

void UIFSMError::proccess()
{
	indicate_set_error_page();
	UIManager::clear();
	pump_stop();
}

bool UIManager::checkKeyboardStop()
{
	return keyboard4x3_is_cancel();
}

bool UIManager::checkKeyboardStart()
{
	return keyboard4x3_is_enter();
}

bool UIManager::checkStart()
{
	return UIManager::checkKeyboardStart() || HAL_GPIO_ReadPin(PUMP_START_GPIO_Port, PUMP_START_Pin);
}
bool UIManager::checkStop()
{
	return UIManager::checkKeyboardStop() || HAL_GPIO_ReadPin(PUMP_STOP_GPIO_Port, PUMP_STOP_Pin);
}

void UIManager::clear()
{
	indicate_set_wait_page();
	keyboard4x3_clear();
}
