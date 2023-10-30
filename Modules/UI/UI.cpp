#include "UI.h"

#include <cmath>
#include <typeinfo>
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


uint32_t UI::lastCard = 0;
uint8_t UI::result[] = {};
bool UI::needLoad = false;
std::shared_ptr<UIFSMBase> UI::ui = std::make_shared<UIFSMInit>();


void UI::UIProccess()
{
	Access::tick();

//	if (UI::checkErrors()) {
//		ui = std::make_shared<UIFSMError>();
//	}

	std::shared_ptr<UIFSMBase> uiPtr;
	if (UI::needToLoad()) {
		uiPtr = std::make_shared<UIFSMLoad>();
	} else {
		uiPtr = ui;
	}
	uiPtr->tick();
}

bool UI::checkErrors()
{
	return general_check_errors();
}

UIFSMBase::UIFSMBase(uint32_t delay)
{
	this->hasErrors = false;
	memset(reinterpret_cast<void*>(&timer), 0, sizeof(timer));
	util_timer_start(&timer, delay);
}

void UIFSMBase::tick()
{
	if (!this->checkState()) {
		return;
	}
	this->proccess();
}

bool UIFSMBase::checkState()
{
	if (this->hasErrors) {
		return false;
	}

	if (UI::needToLoad()) {
		return false;
	}

	if (timer.delay && !util_is_timer_wait(&timer)) {
		UI::ui = std::make_shared<UIFSMStop>();
		pump_stop();
		return false;
	}

	if (UI::checkStop()) {
		UI::ui = std::make_shared<UIFSMStop>();
		pump_stop();
		return false;
	}

//	if (pump_is_working()) {
//		UI::ui = std::make_shared<UIFSMCount>();
//		return;
//	}

	return true;
}

UIFSMInit::UIFSMInit(): UIFSMBase(0)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMInit page");
#endif
	indicate_set_wait_page();
	Access::close();
}

void UIFSMInit::proccess()
{
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMLoad::UIFSMLoad(): UIFSMBase(0)
{
#if UI_BEDUG
//	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMLoad page");
#endif
	indicate_set_load_page();
}

void UIFSMLoad::proccess() { }

UIFSMWait::UIFSMWait(): UIFSMBase(0)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMWait page");
#endif
	keyboard4x3_clear();
	pump_clear();
}

void UIFSMWait::proccess()
{
	indicate_set_wait_page();

	if (Access::isGranted()) {
		UI::ui = std::make_shared<UIFSMInput>();
		UI::setCard(Access::getCard());
	}
}

UIFSMInput::UIFSMInput(): UIFSMBase(UIFSMInput::INPUT_DELAY)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMInput page");
#endif
	memset(UI::result, 0, KEYBOARD4X3_BUFFER_SIZE);
	keyboard4x3_clear();
	pump_clear();
}

void UIFSMInput::proccess()
{
	indicate_set_buffer_page();
	indicate_set_buffer(keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);

	if (UI::checkStart()) {
		UI::ui = std::make_shared<UIFSMStart>();
	}
}

UIFSMStart::UIFSMStart(): UIFSMBase(0)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMStart page");
#endif
}

void UIFSMStart::proccess()
{
	indicate_set_buffer_page();
	uint32_t user_liters       = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}

	uint32_t user_ml = user_liters * liters_multiplier;

	if (user_liters >= GENERAL_SESSION_ML_MIN) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "pump start command");
#endif
		UI::ui = std::make_shared<UIFSMCount>();
		pump_clear();
		pump_set_fuel_ml(user_ml);
		return;
	}

#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "invalid liters value");
#endif
	keyboard4x3_clear();
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMCount::UIFSMCount(): UIFSMBase(UIFSMCount::COUNT_DELAY)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMCount page");
#endif
}

void UIFSMCount::proccess()
{
	indicate_set_buffer_page();
	uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = { 0 };
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}
	uint32_t curr_count = pump_get_fuel_count_ml() / liters_multiplier;
	for (unsigned i = util_get_number_len(curr_count); i > 0; i--) {
		buffer[i-1] = '0' + curr_count % 10;
		curr_count /= 10;
	}
	indicate_set_buffer(buffer, KEYBOARD4X3_BUFFER_SIZE);
	memcpy(UI::result, buffer, KEYBOARD4X3_BUFFER_SIZE);

	if (pump_has_stopped()) {
		UI::ui = std::make_shared<UIFSMResult>();
	}
}

bool UIFSMCount::checkState()
{
	if (UI::checkStop()) {
		keyboard4x3_clear();
		pump_stop();
		UI::ui = std::make_shared<UIFSMResult>();
		return false;
	}

	if (!util_is_timer_wait(&timer)) {
		UI::ui = std::make_shared<UIFSMWait>();
		return false;
	}

	return true;
}

UIFSMStop::UIFSMStop(): UIFSMBase(0)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMStop page");
#endif
	keyboard4x3_clear();
	Access::close();
	pump_stop();
}

void UIFSMStop::proccess()
{
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMResult::UIFSMResult(): UIFSMBase(UIFSMResult::RESULT_DELAY)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMResult page");
#endif
}

void UIFSMResult::proccess()
{
	indicate_set_buffer_page();
	indicate_set_buffer(UI::result, KEYBOARD4X3_BUFFER_SIZE);
	if (!pump_has_stopped()) {
		return;
	}
	if (Access::isGranted()) {
		UI::ui = std::make_shared<UIFSMWait>();
	}
}

bool UIFSMResult::checkState()
{
	if (UI::checkStop()) {
		UI::ui = std::make_shared<UIFSMWait>();
		return false;
	}

	if (!util_is_timer_wait(&timer)) {
		UI::ui = std::make_shared<UIFSMWait>();
		return false;
	}

	return true;
}

UIFSMError::UIFSMError(): UIFSMBase(0)
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UI UIFSMError page");
#endif
	this->hasErrors = true;
}

void UIFSMError::proccess()
{
	indicate_set_error_page();
	pump_stop();
}

bool UI::checkKeyboardStop()
{
	return keyboard4x3_is_cancel();
}

bool UI::checkKeyboardStart()
{
	return keyboard4x3_is_enter();
}

bool UI::needToLoad()
{
	return needLoad && (typeid(*(UI::ui)) == typeid(UIFSMWait));
}

void UI::setLoad()
{
	needLoad = true;
}

void UI::resetLoad()
{
	needLoad = false;
}

void UI::setCard(uint32_t card)
{
	UI::lastCard = card;
}

uint32_t UI::getCard()
{
	return UI::lastCard;
}

bool UI::checkStart()
{
	return UI::checkKeyboardStart() || HAL_GPIO_ReadPin(PUMP_START_GPIO_Port, PUMP_START_Pin);
}
bool UI::checkStop()
{
	return UI::checkKeyboardStop() || HAL_GPIO_ReadPin(PUMP_STOP_GPIO_Port, PUMP_STOP_Pin);
}
