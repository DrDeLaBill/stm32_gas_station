#include "UI.h"

//#include <typeinfo>
#include <cstdlib>
//#include <stdint.h>
#include <cstring>
//#include "stm32f4xx_hal.h"
//
//#include "log.h"
//#include "main.h"
#include "settings.h"

#include "hal_defs.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"
//
#include "Pump.h"
#include "Access.h"


extern settings_t settings;


fsm::FiniteStateMachine<UI::fsm_table> UI::fsm;
utl::Timer UI::timer(UI::BASE_TIMEOUT_MS);
uint32_t UI::card = 0;
bool UI::needLoad = false;
bool UI::error = false;
uint8_t UI::limitBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint8_t UI::currentBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint8_t UI::resultBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint32_t UI::targetMl = 0;
uint32_t UI::lastMl = 0;
uint32_t UI::resultMl = 0;


void UI::proccess()
{
	fsm.proccess();
}

void UI::setLoading()
{
	needLoad = true;
}

void UI::setLoaded()
{
	needLoad = false;
}

void UI::setCard(uint32_t card)
{
    UI::card = card;
}

uint32_t UI::getCard()
{
    return card;
}

bool UI::isEnter()
{
    return keyboard4x3_is_enter() || !HAL_GPIO_ReadPin(PUMP_START_GPIO_Port, PUMP_START_Pin);
}

bool UI::isCancel()
{
    return keyboard4x3_is_cancel() || !HAL_GPIO_ReadPin(PUMP_STOP_GPIO_Port, PUMP_STOP_Pin);
}

void UI::setReboot()
{
	fsm.push_event(reboot_e{});
}

void UI::setError()
{
	if (error) {
		return;
	}
	error = true;
	fsm.push_event(error_e{});
}

void UI::resetError()
{
	if (!error) {
		return;
	}
	error = false;
	fsm.push_event(solved_e{});
}

void UI::resetResultMl()
{
	resultMl = 0;
}

uint32_t UI::getResultMl()
{
	return resultMl;
}

void UI::_init_s::operator ()()
{
	fsm.push_event(success_e{});
}

void UI::_load_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(error_e{});
	}
	if (!needLoad) {
		fsm.push_event(success_e{});
	}
}

void UI::_idle_s::operator ()()
{
	if (Access::isDenied()) {
		fsm.push_event(denied_e{});
	}
	if (Access::isGranted()) {
		fsm.push_event(granted_e{});
	}
	if (needLoad) {
		fsm.push_event(load_e{});
	}

	if (timer.wait()) {
		return;
	}

	// TODO: !!!
//    if (!Pump::hasStopped()) {
//    	fsm.push_event(error_e{});
//		return;
//    }
	// TODO: Add error event handler
}

void UI::_granted_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}
}

void UI::_denied_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}
}

utl::Timer UI::_limit_s::blinkTimer(UI::BLINK_DELAY_MS);
bool UI::_limit_s::limitPage = false;
void UI::_limit_s::operator ()()
{
	if (!blinkTimer.wait()) {
		limitPage ? indicate_set_buffer_page() : indicate_set_limit_page();
		limitPage = !limitPage;
		blinkTimer.start();
	}

	if (strlen(reinterpret_cast<char*>(keyboard4x3_get_buffer()))) {
		fsm.push_event(input_e{});
	}

	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}

	if (isCancel()) {
		fsm.push_event(cancel_e{});
	}
}

void UI::_input_s::operator ()()
{
	if (isCancel()) {
		fsm.push_event(cancel_e{});
	}

	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}

	if (isEnter()) {
		fsm.push_event(start_e{});
	}

    uint8_t number_buffer[KEYBOARD4X3_BUFFER_SIZE] = {};
    uint32_t number = atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
		if (number) {
			number_buffer[i-1] = static_cast<uint8_t>(number % 10) + '0';
		} else {
			number_buffer[i-1] = '_';
		}
		number /= 10;
	}
    indicate_set_buffer(number_buffer, KEYBOARD4X3_BUFFER_SIZE);
}

void UI::_check_s::operator ()() { }

void UI::_wait_count_s::operator ()()
{
    uint32_t liters_multiplier = ML_IN_LTR;
    if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
        liters_multiplier /= util_small_pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
    }
    uint32_t curr_count = Pump::getCurrentMl();
    if (curr_count > targetMl) {
        curr_count = targetMl;
    }

    if (Pump::hasStopped()) {
    	resultMl = lastMl;
    	fsm.push_event(end_e{});
    	return;
    }

	if (!timer.wait()) {
		fsm.push_event(end_e{});
		return;
	}

	if (isCancel()) {
		fsm.push_event(end_e{});
		return;
	}

    if (curr_count != lastMl) {
    	fsm.push_event(start_e{});
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
    memcpy(resultBuffer, buffer, KEYBOARD4X3_BUFFER_SIZE);
    resultMl = 0;
}

void UI::_count_s::operator ()()
{
    uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = { 0 };
    uint32_t curr_count = Pump::getCurrentMl();
    if (curr_count > targetMl) {
        curr_count = targetMl;
    }

    if (curr_count == lastMl) {
    	fsm.push_event(end_e{});
        return;
    }

    lastMl = curr_count;

    uint32_t liters_multiplier = ML_IN_LTR;
    if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
        liters_multiplier /= util_small_pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
    }
    uint32_t tmpCurrCount = curr_count / liters_multiplier;
    for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
    	if (tmpCurrCount || i > KEYBOARD4X3_BUFFER_SIZE - KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT - 1) {
    		buffer[i-1] = static_cast<uint8_t>(tmpCurrCount % 10) + '0';
    		tmpCurrCount /= 10;
    	} else {
    		buffer[i-1] = 0;
    	}
    }
    indicate_set_buffer(buffer, KEYBOARD4X3_BUFFER_SIZE);
    memcpy(resultBuffer, buffer, KEYBOARD4X3_BUFFER_SIZE);
}

void UI::_record_s::operator ()() { }

void UI::_result_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(end_e{});
		return;
	}

	if (isCancel()) {
		fsm.push_event(end_e{});
		return;
	}

    if (Access::isGranted()) {
    	fsm.push_event(granted_e{});
		return;
    }

    if (Access::isDenied()) {
    	fsm.push_event(denied_e{});
		return;
    }
}

void UI::_reboot_s::operator ()()
{
	if (!timer.wait()) {
		fsm.push_event(timeout_e{});
	}
}

void UI::_error_s::operator ()()
{
    Pump::stop();
}

void UI::load_ui_a::operator ()()
{
	timer.changeDelay(BASE_TIMEOUT_MS);
	timer.start();

	indicate_set_load_page();
}

void UI::idle_ui_a::operator ()()
{
	timer.changeDelay(_idle_s::TIMEOUT_MS);
	timer.start();

    indicate_set_wait_page();

    keyboard4x3_disable_light();
    keyboard4x3_clear();

    Access::close();
    Pump::clear();
}

void UI::granted_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(BLINK_DELAY_MS);
	timer.start();

	indicate_set_access_page();

    UI::setCard(Access::getCard());
    Access::close();
}

void UI::denied_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(BLINK_DELAY_MS);
	timer.start();

	indicate_set_denied_page();

    UI::setCard(Access::getCard());
    Access::close();
}

void UI::limit_ui_a::operator ()()
{
	fsm.clear_events();

    uint16_t idx;
    uint32_t residue = 0;
    SettingsStatus status = settings_get_card_idx(UI::getCard(), &idx);
    if (status == SETTINGS_OK && settings.used_liters[idx] < settings.limits[idx]) {
    	residue = settings.limits[idx] - settings.used_liters[idx];
    }
    uint32_t liters_multiplier = ML_IN_LTR;
	if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
		liters_multiplier /= util_small_pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	}
	residue /= liters_multiplier;
    if (util_get_number_len(residue) > KEYBOARD4X3_BUFFER_SIZE) {
    	residue = 999999;
    }

    memset(limitBuffer, 0, sizeof(limitBuffer));
    for (unsigned i = KEYBOARD4X3_BUFFER_SIZE; i > 0; i--) {
    	if (residue || i > KEYBOARD4X3_BUFFER_SIZE - KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT - 1) {
    		limitBuffer[i-1] = static_cast<uint8_t>(residue % 10) + '0';
        	residue /= 10;
    	} else {
    		limitBuffer[i-1] = 0;
    	}
    }

	_limit_s::limitPage = true;

	keyboard4x3_enable_light();
    keyboard4x3_clear();

	indicate_set_limit_page();
	indicate_set_buffer(limitBuffer, KEYBOARD4X3_BUFFER_SIZE);

	timer.changeDelay(BASE_TIMEOUT_MS);
	timer.start();
}

void UI::reset_input_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(BASE_TIMEOUT_MS);
	timer.start();

	memset(currentBuffer, 0 ,sizeof(currentBuffer));

	keyboard4x3_clear();

	indicate_set_buffer_page();
	indicate_clear_buffer();

	targetMl = 0;
	lastMl = 0;
	resultMl = 0;
	Pump::clear();

}

void UI::check_a::operator ()()
{
	fsm.clear_events();

    uint32_t user_input        = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
    uint32_t liters_multiplier = ML_IN_LTR;
    if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
        liters_multiplier /= util_small_pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
    }
    targetMl = user_input * liters_multiplier;

    uint32_t used_liters = 0;
    uint16_t idx;
    if (settings_get_card_idx(UI::getCard(), &idx) == SETTINGS_OK) {
    	used_liters = settings.used_liters[idx];
    } else {
    	fsm.push_event(limit_max_e{});
    }

	if (targetMl + used_liters > settings.limits[idx]) {
		fsm.push_event(limit_max_e{});
	} else if (targetMl < GENERAL_SESSION_ML_MIN) {
		fsm.push_event(limit_min_e{});
	} else {
		fsm.push_event(success_e{});

	    indicate_set_buffer_page();

	    Pump::clear();
	    Pump::setTargetMl(targetMl);
	}
}

void UI::wait_count_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_wait_count_s::TIMEOUT_MS);
	timer.start();
}

void UI::count_a::operator ()()
{
	fsm.clear_events();
}

void UI::record_ui_a::operator ()()
{
	fsm.clear_events();

	resultMl = lastMl;
	fsm.push_event(success_e{});
}

void UI::result_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_result_s::TIMEOUT_MS);
	timer.start();

	keyboard4x3_disable_light();
    keyboard4x3_clear();

    indicate_set_buffer_page();
    indicate_set_buffer(resultBuffer, KEYBOARD4X3_BUFFER_SIZE);

    Access::close();
	Pump::stop();
}

void UI::reboot_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_reboot_s::TIMEOUT_MS);
	timer.start();

	indicate_set_reboot_page();
}

void UI::error_ui_a::operator ()()
{
	fsm.clear_events();
	keyboard4x3_disable_light();
    indicate_set_error_page();
}
