#include "UI.h"

#include <cstdlib>
#include <cstring>

#include "soul.h"
#include "hal_defs.h"
#include "settings.h"
#include "indicate_manager.h"
#include "keyboard4x3_manager.h"

#include "pump.h"
#include "Access.h"
#include "defines.h"


extern settings_t settings;


fsm::FiniteStateMachine<UI::fsm_table> UI::fsm;
utl::Timer UI::timer(UI::BASE_TIMEOUT_MS);
uint32_t UI::card = 0;
uint8_t UI::limitBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint8_t UI::currentBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint8_t UI::resultBuffer[KEYBOARD4X3_BUFFER_SIZE] = {};
uint32_t UI::targetMl = 0;
uint32_t UI::resultMl = 0;


void _ui_show_transition(const char* source, const char* event, const char* target)
{
#ifdef UI_BEDUG
	printTagLog(UI::TAG, "%s %s event -> set %s page", source, event, target);
#endif
}

void UI::proccess()
{
	fsm.proccess();
}

void UI::setCard(uint32_t card)
{
	set_status(NEW_CARD_RECEIVED);
    UI::card = card;
}

uint32_t UI::getCard()
{
    return card;
}

bool UI::isPumpWorking()
{
	return fsm.is_state(wait_count_s{}) || fsm.is_state(count_s{});
}

bool UI::isEnter()
{
#if IS_DEVICE_WITH_KEYBOARD()
    return keyboard4x3_is_enter();
#else
    return false;
#endif
}

bool UI::isCancel()
{
#if IS_DEVICE_WITH_KEYBOARD()
    return keyboard4x3_is_cancel();
#else
    return false;
#endif
}

bool UI::isStart()
{
	return !HAL_GPIO_ReadPin(PUMP_START_GPIO_Port, PUMP_START_Pin);
}

bool UI::isStop()
{
	return !HAL_GPIO_ReadPin(PUMP_STOP_GPIO_Port, PUMP_STOP_Pin);
}

void UI::setReboot()
{
#ifdef UI_BEDUG
	printTagLog(TAG, "Reboot event detected");
#endif
	fsm.push_event(reboot_e{});
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
	_ui_show_transition("init", "success", "load");
	fsm.push_event(success_e{});
}

void UI::_load_s::operator ()()
{
	if (!timer.wait()) {
		_ui_show_transition("load", "error", "error");
		fsm.push_event(error_e{});
		set_error(LOAD_ERROR);
	}
	if (!is_status(LOADING) && is_status(WORKING)) {
		_ui_show_transition("load", "success", "idle");
		fsm.push_event(success_e{});
	}
	if (has_errors()) {
		_ui_show_transition("load", "error", "error");
		fsm.push_event(error_e{});
	}
}

void UI::_idle_s::operator ()()
{
	if (Access::isDenied()) {
		_ui_show_transition("idle", "denied", "denied");
		fsm.push_event(denied_e{});
	}
	if (Access::isGranted()) {
		_ui_show_transition("idle", "granted", "granted");
		fsm.push_event(granted_e{});
	}
	if (is_status(LOADING)) {
		_ui_show_transition("idle", "load", "load");
		fsm.push_event(load_e{});
	}
	if (has_errors()) {
		_ui_show_transition("idle", "error", "error");
		fsm.push_event(error_e{});
	}

	if (timer.wait()) {
		return;
	}

	// TODO: !!!
//    if (!Pump::hasStopped()) {
//    	fsm.push_event(error_e{});
//	    set_error(PUMP_ERROR);
//		return;
//    }
	// TODO: Add error event handler
}

void UI::_granted_s::operator ()()
{
	if (!timer.wait()) {
		_ui_show_transition("granted", "timeout", "limit");
		fsm.push_event(timeout_e{});
	}
}

void UI::_denied_s::operator ()()
{
	if (!timer.wait()) {
		_ui_show_transition("denied", "timeout", "idle");
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
		_ui_show_transition("limit", "input", "input");
		fsm.push_event(input_e{});
	}

	if (isStart()) {
		_ui_show_transition("limit", "start", "check");
		fsm.push_event(start_e{});
	}

	if (!timer.wait()) {
		_ui_show_transition("limit", "timeout", "idle");
		fsm.push_event(timeout_e{});
	}

	if (isCancel() || isStop()) {
		_ui_show_transition("limit", "cancel", "idle");
		fsm.push_event(cancel_e{});
	}
}

void UI::_input_s::operator ()()
{
	if (isCancel() || isStop()) {
		_ui_show_transition("input", "cancel", "idle");
		fsm.push_event(cancel_e{});
	}

	if (!timer.wait()) {
		_ui_show_transition("input", "timeout", "idle");
		fsm.push_event(timeout_e{});
	}

	if (isEnter() || isStart()) {
		_ui_show_transition("input", "enter", "check");

	    uint32_t user_input        = (uint32_t)atoi(reinterpret_cast<char*>(keyboard4x3_get_buffer()));
	    uint32_t liters_multiplier = ML_IN_LTR;
	    if (KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT > 0) {
	        liters_multiplier /= util_small_pow(10, KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT);
	    }
	    targetMl = user_input * liters_multiplier;

		fsm.push_event(enter_e{});
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
    uint32_t curr_count = pump_count_ml();
    if (curr_count > targetMl) {
        curr_count = targetMl;
    }

    if (pump_stopped()) {
#ifdef UI_BEDUG
    	printTagLog(TAG, "pump has been stopped");
#endif
		_ui_show_transition("wait count", "end", "record");
    	fsm.push_event(end_e{});
    	return;
    }

	if (!timer.wait()) {
#ifdef UI_BEDUG
    	printTagLog(TAG, "Time is out. Stopping the pump.");
#endif
		_ui_show_transition("wait count", "end", "record");
		fsm.push_event(end_e{});
		return;
	}

	if (isCancel() || isStop()) {
#ifdef UI_BEDUG
		if (isCancel()) {
			printTagLog(TAG, "Keyboard cansel has been pressed. Stopping the pump.");
		}
		if (isStop()) {
			printTagLog(TAG, "Stop button has been pressed. Stopping the pump.");
		}
#endif
		_ui_show_transition("wait count", "end", "record");
		fsm.push_event(end_e{});
		return;
	}

    if (curr_count != resultMl) {
		_ui_show_transition("wait count", "enter", "count");
    	fsm.push_event(enter_e{});
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
}

void UI::_count_s::operator ()()
{
    uint8_t buffer[KEYBOARD4X3_BUFFER_SIZE] = { 0 };
    uint32_t curr_count = pump_count_ml();
    if (curr_count > targetMl) {
        curr_count = targetMl;
    }

    if (curr_count == resultMl) {
		_ui_show_transition("count", "end", "wait count");
    	fsm.push_event(end_e{});
        return;
    }

    resultMl = curr_count;

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

bool UI::_save_s::loading = false;
void UI::_save_s::operator ()()
{
	if (!timer.wait()) {
		_ui_show_transition("save", "timeout", "error");
		fsm.push_event(timeout_e{});
		return;
	}
	if (!is_status(NEED_SAVE_FINAL_RECORD) && !is_status(NEED_SAVE_SETTINGS)) {
		_ui_show_transition("save", "success", "result");
		fsm.push_event(success_e{});
		return;
	}
	if (loading) {
		return;
	}
	if (Access::isGranted()) {
		UI::setCard(Access::getCard());
		_ui_show_transition("save", "granted", "save");
		fsm.push_event(granted_e{});
		return;
	}
	if (Access::isDenied()) {
		_ui_show_transition("save", "denied", "save");
		fsm.push_event(denied_e{});
		return;
	}
}

void UI::_result_s::operator ()()
{
	if (has_errors()) {
		_ui_show_transition("result", "error", "-----");
		fsm.push_event(error_e{});
		return;
	}

	if (!timer.wait()) {
		_ui_show_transition("result", "end", "load");
		fsm.push_event(end_e{});
		return;
	}

	if (isCancel() || isStop()) {
		_ui_show_transition("result", "end", "load");
		fsm.push_event(end_e{});
		return;
	}

    if (Access::isGranted()) {
		_ui_show_transition("result", "granted", "granted");
    	fsm.push_event(granted_e{});
		return;
    }

    if (Access::isDenied()) {
		_ui_show_transition("result", "denied", "denied");
    	fsm.push_event(denied_e{});
		return;
    }
}

void UI::_reboot_s::operator ()()
{
	if (!timer.wait()) {
		_ui_show_transition("reboot", "timeout", "idle");
		fsm.push_event(timeout_e{});
	}
}

void UI::_error_s::operator ()()
{
    pump_stop();
	if (!has_errors()) {
		_ui_show_transition("error", "solved", "load");
		fsm.push_event(solved_e{});
	}
}

void UI::load_ui_a::operator ()()
{
	timer.changeDelay(BASE_TIMEOUT_MS);
	timer.start();

	indicate_set_load_page();
}

void UI::idle_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_idle_s::TIMEOUT_MS);
	timer.start();

    indicate_set_wait_page();

#if IS_DEVICE_WITH_KEYBOARD()
    keyboard4x3_disable_light();
    keyboard4x3_clear();
#endif

    Access::close();
    // TODO: Pump::clear();
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

#if IS_DEVICE_WITH_KEYBOARD()
	keyboard4x3_enable_light();
    keyboard4x3_clear();
#endif

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

#if IS_DEVICE_WITH_KEYBOARD()
	keyboard4x3_clear_enter();
#endif

	indicate_set_buffer_page();
	indicate_clear_buffer();

	targetMl = 0;
	resultMl = 0;
    // TODO: Pump::clear();
}

void UI::check_a::operator ()()
{
	fsm.clear_events();

    uint32_t used_liters = 0;
    uint16_t idx;
    uint32_t limit = 0;
    if (settings_get_card_idx(UI::getCard(), &idx) == SETTINGS_OK) {
    	used_liters = settings.used_liters[idx];
    	limit = settings.limits[idx];
    } else {
    	_ui_show_transition("check", "limit max", "limit");
    	fsm.push_event(limit_max_e{});
    }

    if (targetMl + used_liters > limit) {
    	_ui_show_transition("check", "limit max", "limit");
		fsm.push_event(limit_max_e{});
	} else if (targetMl < GENERAL_SESSION_ML_MIN) {
    	_ui_show_transition("check", "limit min", "input");
		fsm.push_event(limit_min_e{});
	} else {
    	_ui_show_transition("check", "success", "wait count");
		fsm.push_event(success_e{});

	    indicate_set_buffer_page();

	    // TODO: Pump::clear();
		pump_start();
	    set_pump_target(targetMl);

	    set_status(NEED_INIT_RECORD_TMP);
	}
}

void UI::start_a::operator ()()
{
	fsm.clear_events();

    uint16_t idx = 0;

	if (settings_get_card_idx(UI::getCard(), &idx) != SETTINGS_OK) {
		_ui_show_transition(" action start", "limit max", "limit");
		fsm.push_event(limit_max_e{});
		return;
	}

	if (settings.used_liters[idx] >= settings.limits[idx]) {
		_ui_show_transition(" action start", "limit max", "limit");
		fsm.push_event(limit_max_e{});
		return;
	}

	uint32_t used_liters = settings.used_liters[idx];
	uint32_t limit = settings.limits[idx];
	targetMl = used_liters >= limit ? 0 : limit - used_liters;

	_ui_show_transition("action start", "success", "wait count");
	fsm.push_event(success_e{});

	indicate_set_buffer_page();

    // TODO: Pump::clear();
	pump_start();
	set_pump_target(targetMl);

	set_status(NEED_INIT_RECORD_TMP);
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

	_ui_show_transition("record", "success", "save");
	fsm.push_event(success_e{});
}

void UI::start_save_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_save_s::TIMEOUT_MS);
	timer.start();

    Access::close();
    pump_stop();

	set_status(NEED_SAVE_FINAL_RECORD);
	_save_s::loading = false;
}

void UI::show_load_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_save_s::TIMEOUT_MS);
	timer.start();

	indicate_set_load_page();
	_save_s::loading = true;
}

void UI::result_ui_a::operator ()()
{
	fsm.clear_events();

	timer.changeDelay(_result_s::TIMEOUT_MS);
	timer.start();

#if IS_DEVICE_WITH_KEYBOARD()
	keyboard4x3_disable_light();
    keyboard4x3_clear();
#endif

    indicate_set_buffer_page();
    indicate_set_buffer(resultBuffer, KEYBOARD4X3_BUFFER_SIZE);

	if (Access::isGranted()) {
		_ui_show_transition("result", "granted", "granted");
		fsm.push_event(granted_e{});
	    UI::setCard(Access::getCard());
	}
	if (Access::isDenied()) {
		_ui_show_transition("result", "denied", "denied");
		fsm.push_event(denied_e{});
	}

	pump_stop();
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
#if IS_DEVICE_WITH_KEYBOARD()
	keyboard4x3_disable_light();
#endif
    indicate_set_error_page();
    set_status(INTERNAL_ERROR);
}
