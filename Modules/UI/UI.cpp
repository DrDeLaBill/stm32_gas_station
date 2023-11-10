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
    memset(reinterpret_cast<void*>(&reset_timer), 0, sizeof(reset_timer));
    util_timer_start(&reset_timer, delay);
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

    if (reset_timer.delay && !util_is_timer_wait(&reset_timer)) {
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
    keyboard4x3_disable_light();
    keyboard4x3_clear();
    Pump::clear();
}

void UIFSMWait::proccess()
{
    indicate_set_wait_page();

    if (Access::isGranted()) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWait->UIFSMAccess");
#endif
        UI::setCard(Access::getCard());
        Access::close();
        UI::ui = std::make_shared<UIFSMAccess>();
    }

    if (Access::isDenied()) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMWait->UIFSMDenied");
#endif
        UI::setCard(Access::getCard());
        Access::close();
        UI::ui = std::make_shared<UIFSMDenied>();
    }
}

UIFSMAccess::UIFSMAccess(): UIFSMBase(0)
{
	indicate_set_access_page();
	util_timer_start(&page_timer, UIFSMAccess::PAGE_DELAY);
}

void UIFSMAccess::proccess()
{
	if (!util_is_timer_wait(&page_timer)) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMAccess->UIFSMLimit");
#endif
        UI::ui = std::make_shared<UIFSMLimit>();
	}
}

UIFSMDenied::UIFSMDenied(): UIFSMBase(0)
{
	indicate_set_denied_page();
	util_timer_start(&page_timer, UIFSMDenied::PAGE_DELAY);
}

void UIFSMDenied::proccess()
{
	if (!util_is_timer_wait(&page_timer)) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMDenied->UIFSMWait");
#endif
        UI::ui = std::make_shared<UIFSMWait>();
	}
}

UIFSMLimit::UIFSMLimit(): UIFSMBase(UIFSMLimit::LIMIT_DELAY)
{
	keyboard4x3_enable_light();
    keyboard4x3_clear();
    uint32_t used_liters = 0;
    uint16_t idx;
    uint32_t residue = 0;
    SettingsDB::SettingsStatus status = settings.getCardIdx(UI::getCard(), &idx);
    if (status == SettingsDB::SETTINGS_OK && used_liters < settings.settings.limits[idx]) {
    	residue = settings.settings.limits[idx] - settings.settings.used_liters[idx];
    }
    if (util_get_number_len(residue) > KEYBOARD4X3_BUFFER_SIZE) {
    	residue = 999999;
    }

    memset(number_buffer, 0, sizeof(number_buffer));
    for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
    	if (residue) {
    		number_buffer[i-1] = residue % 10 + '0';
        	residue /= 10;
    	} else {
    		number_buffer[i-1] = 0;
    	}
    }

	this->isLimitPage = true;
	indicate_set_limit_page();
	util_timer_start(&page_timer, UIFSMLimit::BLINK_DELAY);
}

void UIFSMLimit::proccess()
{
	if (!util_is_timer_wait(&page_timer)) {
		this->isLimitPage ? indicate_set_buffer_page() : indicate_set_limit_page();
	    indicate_set_buffer(number_buffer, KEYBOARD4X3_BUFFER_SIZE);
		this->isLimitPage = !this->isLimitPage;
		util_timer_start(&page_timer, UIFSMLimit::BLINK_DELAY);
	}

	if (strlen(reinterpret_cast<char*>(keyboard4x3_get_buffer()))) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMLimit->UIFSMInput");
#endif
        UI::ui = std::make_shared<UIFSMInput>();
	}
}

UIFSMInput::UIFSMInput(): UIFSMBase(UIFSMInput::INPUT_DELAY)
{
    UIFSMBase::targetMl = 0;
    memset(UI::result, 0, KEYBOARD4X3_BUFFER_SIZE);
    Pump::clear();
}

void UIFSMInput::proccess()
{
    indicate_set_buffer_page();

    uint8_t number_buffer[KEYBOARD4X3_BUFFER_SIZE] = {};
    uint32_t number = atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
		if (number) {
			number_buffer[i-1] = number % 10 + '0';
		} else {
			number_buffer[i-1] = '_';
		}
		number /= 10;
	}
    indicate_set_buffer(number_buffer, KEYBOARD4X3_BUFFER_SIZE);

    if (UI::checkStart()) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMInput->UIFSMStart");
#endif
        UI::ui = std::make_shared<UIFSMStart>();
    }
}

UIFSMStart::UIFSMStart(): UIFSMBase(0)
{
    indicate_set_buffer_page();
    uint32_t user_input        = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
    uint32_t liters_multiplier = ML_IN_LTR;
    if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
        liters_multiplier /= pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
    }

    UIFSMBase::targetMl = user_input * liters_multiplier;
}

void UIFSMStart::proccess()
{
    uint32_t used_liters = 0;
    uint16_t idx;
    if (settings.getCardIdx(UI::getCard(), &idx) == SettingsDB::SETTINGS_OK) {
    	used_liters = settings.settings.used_liters[idx];
    } else {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStart->UIFSMRError");
#endif
        UI::ui = std::make_shared<UIFSMError>();
        return;
    }

	if (UIFSMBase::targetMl + used_liters > settings.settings.limits[idx]) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStart->UIFSMLimit");
#endif
        UI::ui = std::make_shared<UIFSMLimit>();
        return;
	}

    if (UIFSMBase::targetMl >= GENERAL_SESSION_ML_MIN) {
#if UI_BEDUG
        LOG_TAG_BEDUG(UI::TAG, "Set UIFSMStart->UIFSMWaitCount");
#endif
        UI::ui = std::make_shared<UIFSMWaitCount>(0);
        uint8_t number_buffer[KEYBOARD4X3_BUFFER_SIZE] = {};
        memset(number_buffer, '0', KEYBOARD4X3_BUFFER_SIZE);
        indicate_set_buffer(number_buffer, KEYBOARD4X3_BUFFER_SIZE);
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

    if (curr_count) {
    	return;
    }
    uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = {};
	for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
		if (i > KEYBOARD4X3_BUFFER_SIZE - KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT - 1) {
			buffer[i-1] = '0';
		}
	}
    indicate_set_buffer(buffer, KEYBOARD4X3_BUFFER_SIZE);
    memcpy(UI::result, buffer, KEYBOARD4X3_BUFFER_SIZE);
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

    if (!util_is_timer_wait(&reset_timer)) {
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

    uint32_t tmpCurrCount = curr_count;
    for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
    	if (curr_count) {
    		buffer[i-1] = tmpCurrCount % 10 + '0';
    		tmpCurrCount /= 10;
    	} else {
    		buffer[i-1] = 0;
    	}
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
	keyboard4x3_disable_light();
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
    Pump::stop();
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

    if (!util_is_timer_wait(&reset_timer)) {
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
	keyboard4x3_disable_light();
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
