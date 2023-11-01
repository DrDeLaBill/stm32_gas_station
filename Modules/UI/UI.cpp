#include "UI.h"

#include <cmath>
#include <typeinfo>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "stm32f4xx_hal.h"

#include "Pump.h"
#include "Access.h"
#include "SettingsDB.h"

#include "main.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"


extern SettingsDB settings;


uint32_t UI::lastCard = 0;
uint8_t UI::result[] = {};
bool UI::needLoad = false;
std::shared_ptr<UIFSMBase> UI::ui = std::make_shared<UIFSMInit>();

uint32_t UIFSMBase::targetMl = 0;


void UI::UIProccess()
{
	Access::tick();

	if (ui->hasError()) {
		return;
	}

	if (UI::checkErrors()){
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIProccess->UIFSMStop");
#endif
		ui = std::make_shared<UIFSMError>();
		return;
	}

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

bool UIFSMBase::hasError()
{
	return this->hasErrors;
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
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMBase->UIFSMStop (check timer)");
#endif
		UI::ui = std::make_shared<UIFSMStop>();
		Pump::stop();
		return false;
	}

	if (UI::checkStop()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMBase->UIFSMStop (check stop)");
#endif
		UI::ui = std::make_shared<UIFSMStop>();
		Pump::stop();
		return false;
	}

	return true;
}

UIFSMInit::UIFSMInit(): UIFSMBase(0)
{
	indicate_set_wait_page();
	Access::close();
}

void UIFSMInit::proccess()
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UIFSMInit->UIFSMWait");
#endif
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMLoad::UIFSMLoad(): UIFSMBase(0)
{
	indicate_set_load_page();
}

void UIFSMLoad::proccess() { }

UIFSMWait::UIFSMWait(): UIFSMBase(0)
{
	keyboard4x3_clear();
	Pump::clear();
}

void UIFSMWait::proccess()
{
	indicate_set_wait_page();

	if (Access::isGranted()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWait->UIFSMInput");
#endif
		UI::ui = std::make_shared<UIFSMInput>();
		UI::setCard(Access::getCard());
	}
}

UIFSMInput::UIFSMInput(): UIFSMBase(UIFSMInput::INPUT_DELAY)
{
	UIFSMBase::targetMl = 0;
	memset(UI::result, 0, KEYBOARD4X3_BUFFER_SIZE);
	keyboard4x3_clear();
	Pump::clear();
}

void UIFSMInput::proccess()
{
	indicate_set_buffer_page();
	indicate_set_buffer(keyboard4x3_get_buffer(), KEYBOARD4X3_BUFFER_SIZE);

	if (UI::checkStart()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMInput->UIFSMStart");
#endif
		UI::ui = std::make_shared<UIFSMStart>();
	}
}

UIFSMStart::UIFSMStart(): UIFSMBase(0){ }

void UIFSMStart::proccess()
{
	indicate_set_buffer_page();
	uint32_t user_input        = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}

	UIFSMBase::targetMl = user_input * liters_multiplier;

	if (UIFSMBase::targetMl >= GENERAL_SESSION_ML_MIN) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStart->UIFSMWaitCount");
#endif
		UI::ui = std::make_shared<UIFSMWaitCount>(0);
		Pump::clear();
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "pump set target");
#endif
		Pump::setTargetMl(UIFSMBase::targetMl);
		return;
	}

#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "invalid liters value");
	LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStart->UIFSMWait");
#endif
	keyboard4x3_clear();
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMWaitCount::UIFSMWaitCount(uint32_t lastMl): UIFSMBase(UIFSMCount::COUNT_DELAY), lastMl(lastMl) {}

void UIFSMWaitCount::proccess()
{
	indicate_set_buffer_page();

	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}
	uint32_t curr_count = Pump::getCurrentMl();
	if (curr_count > UIFSMBase::targetMl) {
		curr_count = UIFSMBase::targetMl;
	}
	curr_count /= liters_multiplier;

	if (Pump::hasStopped()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWaitCount->UIFSMResult");
#endif
		UI::ui = std::make_shared<UIFSMResult>();
	}

	if (curr_count != this->lastMl) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWaitCount->UIFSMCount");
#endif
		UI::ui = std::make_shared<UIFSMCount>(this->lastMl);
		return;
	}
}

bool UIFSMWaitCount::checkState()
{
	if (UI::checkStop()) {
		keyboard4x3_clear();
		Pump::stop();
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWaitCount->UIFSMResult (check stop)");
#endif
		UI::ui = std::make_shared<UIFSMResult>();
		return false;
	}

	if (!util_is_timer_wait(&timer)) {
		Pump::stop();
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWaitCount->UIFSMResult (timer)");
#endif
		UI::ui = std::make_shared<UIFSMResult>();
		return false;
	}

	return true;
}

UIFSMCount::UIFSMCount(uint32_t lastMl): UIFSMWaitCount(lastMl) { }

void UIFSMCount::proccess()
{
	indicate_set_buffer_page();

	uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = { 0 };
	uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}
	uint32_t curr_count = Pump::getCurrentMl();
	if (curr_count > UIFSMBase::targetMl) {
		curr_count = UIFSMBase::targetMl;
	}
	curr_count /= liters_multiplier;

	if (curr_count == this->lastMl) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMCount->UIFSMWaitCount");
#endif
		UI::ui = std::make_shared<UIFSMWaitCount>(this->lastMl);
		return;
	}

	this->lastMl = curr_count;

	for (unsigned i = util_get_number_len(curr_count); i > 0; i--) {
		buffer[i-1] = '0' + curr_count % 10;
		curr_count /= 10;
	}
	indicate_set_buffer(buffer, KEYBOARD4X3_BUFFER_SIZE);
	memcpy(UI::result, buffer, KEYBOARD4X3_BUFFER_SIZE);

	if (Pump::hasStopped()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMCount->UIFSMResult");
#endif
		UI::ui = std::make_shared<UIFSMResult>();
	}
}

bool UIFSMCount::checkState()
{
	if (UI::checkStop()) {
		keyboard4x3_clear();
		Pump::stop();
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMCount->UIFSMResult");
#endif
		UI::ui = std::make_shared<UIFSMResult>();
		return false;
	}

	return true;
}

UIFSMStop::UIFSMStop(): UIFSMBase(0)
{
	keyboard4x3_clear();
	Access::close();
	Pump::stop();
}

void UIFSMStop::proccess()
{
#if UI_BEDUG
	LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStop->UIFSMWait");
#endif
	UI::ui = std::make_shared<UIFSMWait>();
}

UIFSMResult::UIFSMResult(): UIFSMBase(UIFSMResult::RESULT_DELAY)
{
	indicate_set_buffer(UI::result, KEYBOARD4X3_BUFFER_SIZE);
	Access::close();
}

void UIFSMResult::proccess()
{
	indicate_set_buffer_page();
	if (!Pump::hasStopped()) {
		return;
	}
	if (Access::isGranted()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMResult->UIFSMWait");
#endif
		UI::ui = std::make_shared<UIFSMWait>();
	}
}

bool UIFSMResult::checkState()
{
	if (UI::checkStop()) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMResult->UIFSMWait");
#endif
		UI::ui = std::make_shared<UIFSMWait>();
		return false;
	}

	if (!util_is_timer_wait(&timer)) {
#if UI_BEDUG
		LOG_TAG_BEDUG(UI::TAG, "Set UIFSMResult->UIFSMWait");
#endif
		UI::ui = std::make_shared<UIFSMWait>();
		return false;
	}

	return true;
}

UIFSMError::UIFSMError(): UIFSMBase(0)
{
	this->hasErrors = true;
	indicate_set_error_page();
	Pump::stop();
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

bool UI::checkGunOnBase()
{
	return HAL_GPIO_ReadPin(GUN_SWITCH_GPIO_Port, GUN_SWITCH_Pin);
}
