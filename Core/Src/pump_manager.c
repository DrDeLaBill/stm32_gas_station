/* Copyright © 2023 Georgy E. All rights reserved. */

#include "pump_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


#define PUMP_MD212_CYCLE_IMPULSES_COUNT ((int32_t)200)
#define PUMP_MD212_CYCLE_ML_VALUE       ((int32_t)1000)
#define PUMP_MD212_CYCLE_INACCURACY     ((int32_t)20)
#define PUMP_MD212_MEASURE_DELAY_MS     ((uint32_t)300)

#define PUMP_ADC_READ_TIMEOUT_MS        ((uint32_t)100)
#define PUMP_ADC_MEASURE_DELAY_MS       ((uint32_t)100)
#define PUMP_ADC_OVERHEAD               ((uint32_t)4000)
#define PUMP_ADC_VALVE_MIN              ((uint32_t)1000)
#define PUMP_ADC_VALVE_MAX              ((uint32_t)2000)
#define PUMP_ADC_PUMP_MIN               ((uint32_t)1000)
#define PUMP_ADC_PUMP_MAX               ((uint32_t)2000)
//
#define PUMP_SESSION_ML_MAX             ((uint32_t)50000)
#define PUMP_SESSION_ML_MIN             ((uint32_t)1000)
#define PUMP_CHECK_START_DELAY_MS       ((uint32_t)5000)
#define PUMP_CHECK_STOP_DELAY_MS        ((uint32_t)5000)
#define PUMP_MEASURE_BUFFER_SIZE        ((uint8_t)10)

#define PUMP_SLOW_ML_VALUE              ((uint32_t)5000)

#define PUMP_ENCODER_MIDDLE             ((uint32_t)0xFFFF / 2)


typedef struct _pump_state_t {
    uint32_t     target_ml;
    void         (*fsm_pump_state) (void);
    void         (*record_handler) (void);
    bool         pump_error;

    uint32_t     pump_counter;
    uint32_t     valve_measure_buf[PUMP_MEASURE_BUFFER_SIZE];
    uint32_t     pump_measure_buf [PUMP_MEASURE_BUFFER_SIZE];
    int32_t      md212_measure_buf[PUMP_MEASURE_BUFFER_SIZE];

    uint32_t     ml_current_count;

    util_timer_t wait_timer;
    util_timer_t error_timer;

    bool         need_start;
    bool         need_stop;
} pump_state_t;


void _pump_set_fsm_state(void (*new_fsm_state) (void));

void _pump_fsm_init();
void _pump_fsm_wait_liters();
void _pump_fsm_wait_start();
void _pump_fsm_start();
void _pump_fsm_check_start();
void _pump_fsm_work();
void _pump_fsm_error();
void _pump_fsm_stop();
void _pump_fsm_check_stop();
void _pump_fsm_record();

void _pump_set_pump_enable(GPIO_PinState enable_state);
void _pump_set_valve1_enable(GPIO_PinState enable_state);
void _pump_set_valve2_enable(GPIO_PinState enable_state);
void _pump_reset_state();

uint32_t _pump_get_adc_valve_current();
uint32_t _pump_get_adc_pump_current();
uint32_t _pump_get_average(uint32_t* data, uint32_t len);
uint32_t _pump_get_average_difference(int32_t* data, uint32_t len);

bool _is_pump_not_working();
bool _is_pump_and_valve_enabled();
bool _is_pump_and_valve_operable();

int32_t _get_pump_encoder_current_ticks();
void _reset_pump_encoder();


static const char* PUMP_TAG = "PMP";

pump_state_t pump_state = {
    .target_ml         = 0,
    .fsm_pump_state    = _pump_fsm_init,
    .record_handler    = NULL,
    .pump_error        = false,

    .pump_counter      = 0,
    .pump_measure_buf  = { 0 },
    .valve_measure_buf = { 0 },
    .md212_measure_buf = { 0 },

    .wait_timer        = { 0 },
    .error_timer       = { 0 },

	.need_start        = false,
	.need_stop         = false
};



void pump_proccess()
{
	if (pump_state.fsm_pump_state == NULL) {
		_pump_reset_state();
	}
    pump_state.fsm_pump_state();
}

void pump_set_fuel_ml(uint32_t amount_ml)
{
    if (pump_state.pump_error) {
        return;
    }
    if (amount_ml > PUMP_SESSION_ML_MAX) {
        amount_ml = PUMP_SESSION_ML_MAX;
    }
    if (amount_ml < PUMP_SESSION_ML_MIN) {
        return;
    }
    pump_state.target_ml = amount_ml;
}

void pump_start()
{
	pump_state.need_start = true;
}

void pump_stop()
{
	pump_state.need_stop = true;
}

bool pump_is_working()
{
	return pump_state.fsm_pump_state == _pump_fsm_work;
}

bool pump_is_free()
{
	return pump_state.fsm_pump_state == _pump_fsm_wait_liters ||
		   pump_state.fsm_pump_state == _pump_fsm_init;
}

bool pump_has_error()
{
	return pump_state.pump_error;
}

void pump_set_record_handler(void (*pump_record_handler) (void))
{
    if (pump_record_handler != NULL) {
    	pump_state.record_handler = pump_record_handler;
    }
}

uint32_t pump_get_fuel_count_ml()
{
    return pump_state.ml_current_count;
}

void _pump_set_fsm_state(void (*new_fsm_state) (void))
{
	if (new_fsm_state == NULL) {
		return;
	}
	memset(pump_state.pump_measure_buf, 0, sizeof(pump_state.pump_measure_buf));
	memset(pump_state.valve_measure_buf, 0, sizeof(pump_state.valve_measure_buf));
    util_timer_start(&pump_state.error_timer, 0);
    util_timer_start(&pump_state.wait_timer, 0);
	pump_state.fsm_pump_state = new_fsm_state;
	pump_state.pump_counter = 0;
	pump_state.need_start   = false;
	pump_state.need_stop    = false;
}

void _pump_fsm_init()
{
	_pump_fsm_stop();
#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_wait_liters", HAL_GetTick());
#endif
	_pump_set_fsm_state(_pump_fsm_wait_liters);
}

void _pump_fsm_wait_liters()
{
	if (pump_state.target_ml > 0) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_wait_start", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_wait_start);
		pump_start();
	}
}

void _pump_fsm_wait_start()
{
	if (pump_state.need_stop) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_stop", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_stop);
		return;
	}

	if (pump_state.need_start && pump_state.target_ml > 0) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_start", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_start);
		return;
	}

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    if (pump_state.pump_counter < __arr_len(pump_state.pump_measure_buf)) {
        util_timer_start(&pump_state.wait_timer, PUMP_ADC_MEASURE_DELAY_MS);
        pump_state.pump_measure_buf[pump_state.pump_counter]  = _pump_get_adc_pump_current();
        pump_state.valve_measure_buf[pump_state.pump_counter] = _pump_get_adc_valve_current();
        pump_state.pump_counter++;
        return;
    }

    pump_state.pump_counter = 0;

    if (_is_pump_and_valve_enabled() || !_is_pump_and_valve_operable()) {
#if PUMP_BEDUG
    	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
    	_pump_set_fsm_state(_pump_fsm_error);
    	return;
    }
}

void _pump_fsm_start()
{
	_reset_pump_encoder();

    _pump_set_pump_enable(GPIO_PIN_SET);
    _pump_set_valve1_enable(GPIO_PIN_SET);
    _pump_set_valve2_enable(GPIO_PIN_SET);

#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_check_start", HAL_GetTick());
#endif
    _pump_set_fsm_state(_pump_fsm_check_start);

    pump_state.ml_current_count = 0;
    _reset_pump_encoder();
    util_timer_start(&pump_state.error_timer, PUMP_CHECK_START_DELAY_MS);
}

void _pump_fsm_check_start()
{
    if (!util_is_timer_wait(&pump_state.error_timer)) {
#if PUMP_BEDUG
    	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
        _pump_set_fsm_state(_pump_fsm_error);
        return;
    }

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    util_timer_start(&pump_state.wait_timer, PUMP_ADC_MEASURE_DELAY_MS);
    pump_state.pump_measure_buf[pump_state.pump_counter]  = _pump_get_adc_pump_current();
    pump_state.valve_measure_buf[pump_state.pump_counter] = _pump_get_adc_valve_current();
    pump_state.pump_counter++;

    if (pump_state.pump_counter < __arr_len(pump_state.pump_measure_buf)) {
        return;
    }

    if (!_is_pump_and_valve_enabled() || !_is_pump_and_valve_operable()) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_error);
		return;
	}

#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_work", HAL_GetTick());
#endif
	_pump_set_fsm_state(_pump_fsm_work);
}

void _pump_fsm_work()
{
	if (pump_state.need_stop) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_stop", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_stop);
		return;
	}

	if (util_is_timer_wait(&pump_state.wait_timer)) {
		return;
	}

	util_timer_start(&pump_state.wait_timer, PUMP_MD212_MEASURE_DELAY_MS);
	pump_state.md212_measure_buf[pump_state.pump_counter]  = _get_pump_encoder_current_ticks();
	pump_state.pump_measure_buf[pump_state.pump_counter]   = _pump_get_adc_pump_current();
	pump_state.valve_measure_buf[pump_state.pump_counter]  = _pump_get_adc_valve_current();
	pump_state.pump_counter++;

	if (pump_state.pump_counter < __arr_len(pump_state.md212_measure_buf)) {
		return;
	}

    pump_state.pump_counter = 0;

//	if (_is_pump_not_working()) {
//#if PUMP_BEDUG
//		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
//#endif
//		_pump_set_fsm_state(_pump_fsm_error);
//		return;
//	}

	if (!_is_pump_and_valve_enabled() || !_is_pump_and_valve_operable()) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
		_pump_set_fsm_state(_pump_fsm_error);
		return;
	}

	int32_t ml_current_count = (_get_pump_encoder_current_ticks() * PUMP_MD212_CYCLE_ML_VALUE) / PUMP_MD212_CYCLE_IMPULSES_COUNT;
	if (ml_current_count < 0) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | pump isn't working: current gas ticks=%ld; target=%lu", HAL_GetTick(), ml_current_count, pump_state.target_ml);
#endif
		_reset_pump_encoder();
		return;
	}

#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | current gas ml: %lu (%ld ticks); target: %lu", HAL_GetTick(), pump_state.ml_current_count, ml_current_count, pump_state.target_ml);
#endif

	pump_state.ml_current_count += ml_current_count;
	_reset_pump_encoder();

	uint32_t ml_fast_count_target = pump_state.target_ml;
	if (ml_fast_count_target > PUMP_SLOW_ML_VALUE) { // TODO: if ml_current_count < 0 -> error
		ml_fast_count_target -= PUMP_SLOW_ML_VALUE;
	}
	if (pump_state.ml_current_count >= ml_fast_count_target) {
		_pump_set_valve1_enable(GPIO_PIN_RESET);
	}

	if (pump_state.ml_current_count >= (int32_t)pump_state.target_ml) {
#if PUMP_BEDUG
		LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_stop", HAL_GetTick());
#endif
		_pump_set_fsm_state(&_pump_fsm_stop);
	}
}

void _pump_fsm_stop()
{
    _pump_set_pump_enable(GPIO_PIN_RESET);
    _pump_set_valve1_enable(GPIO_PIN_RESET);
    _pump_set_valve2_enable(GPIO_PIN_RESET);

#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_check_stop", HAL_GetTick());
#endif
    _pump_set_fsm_state(_pump_fsm_check_stop);
    util_timer_start(&pump_state.error_timer, PUMP_CHECK_STOP_DELAY_MS);
}

void _pump_fsm_check_stop()
{
    if (!util_is_timer_wait(&pump_state.error_timer)) {
#if PUMP_BEDUG
    	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
        _pump_set_fsm_state(_pump_fsm_error);
        return;
    }

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    if (pump_state.pump_counter < __arr_len(pump_state.pump_measure_buf)) {
        util_timer_start(&pump_state.wait_timer, PUMP_ADC_MEASURE_DELAY_MS);
        pump_state.pump_measure_buf[pump_state.pump_counter] = _pump_get_adc_pump_current();
        pump_state.valve_measure_buf[pump_state.pump_counter++] = _pump_get_adc_valve_current();
        return;
    }

    if (_is_pump_and_valve_enabled() || !_is_pump_and_valve_operable()) {
#if PUMP_BEDUG
    	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_error", HAL_GetTick());
#endif
        _pump_set_fsm_state(_pump_fsm_error);
        return;
    }

#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_record", HAL_GetTick());
#endif
	_pump_set_fsm_state(_pump_fsm_record);
}

void _pump_fsm_record()
{
	if (pump_state.ml_current_count > 0) {
		pump_state.record_handler();
	}
#if PUMP_BEDUG
	LOG_TAG_BEDUG(PUMP_TAG, "%08lu ms | set _pump_fsm_wait_liters", HAL_GetTick());
#endif
    _pump_set_fsm_state(_pump_fsm_wait_liters);
    pump_state.target_ml = 0;

    pump_state.ml_current_count = 0;
    _reset_pump_encoder();
}

void _pump_fsm_error()
{
	pump_state.pump_error = true;
	pump_state.record_handler = NULL;

	_pump_set_pump_enable(GPIO_PIN_RESET);
	_pump_set_valve1_enable(GPIO_PIN_RESET);
	_pump_set_valve2_enable(GPIO_PIN_RESET);

	_reset_pump_encoder();
}

void _pump_set_pump_enable(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(PUMP_GPIO_Port, PUMP_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, enable_state);
}

void _pump_set_valve1_enable(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(VALVE1_GPIO_Port, VALVE1_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(VALVE1_GPIO_Port, VALVE1_Pin, enable_state);
}

void _pump_set_valve2_enable(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(VALVE2_GPIO_Port, VALVE2_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(VALVE2_GPIO_Port, VALVE2_Pin, enable_state);
}

void _pump_reset_state()
{
    void (*pump_stop_handler) (void) ;
    pump_stop_handler = pump_state.record_handler;

    memset((uint8_t*)&pump_state, 0, sizeof(pump_state));

    pump_state.fsm_pump_state    = &_pump_fsm_stop;
    pump_state.record_handler = pump_stop_handler;

    _reset_pump_encoder();
}

uint32_t _pump_get_adc_valve_current()
{
    ADC_ChannelConfTypeDef conf = {
        .Channel      = VALVE_ADC_CHANNEL,
        .Rank         = 1,
        .SamplingTime = ADC_SAMPLETIME_28CYCLES,
    };
    if (HAL_ADC_ConfigChannel(&VALVE_ADC, &conf) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&VALVE_ADC);
    HAL_ADC_PollForConversion(&VALVE_ADC, PUMP_ADC_READ_TIMEOUT_MS);
    uint32_t value = HAL_ADC_GetValue(&VALVE_ADC);
    HAL_ADC_Stop(&VALVE_ADC);

    return value;
}

uint32_t _pump_get_adc_pump_current()
{
    ADC_ChannelConfTypeDef conf = {
        .Channel      = PUMP_ADC_CHANNEL,
        .Rank         = 1,
        .SamplingTime = ADC_SAMPLETIME_28CYCLES,
    };
    if (HAL_ADC_ConfigChannel(&PUMP_ADC, &conf) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&PUMP_ADC);
    HAL_ADC_PollForConversion(&PUMP_ADC, PUMP_ADC_READ_TIMEOUT_MS);
    uint32_t value = HAL_ADC_GetValue(&PUMP_ADC);
    HAL_ADC_Stop(&PUMP_ADC);

    return value;
}

uint32_t _pump_get_average(uint32_t* data, uint32_t len)
{
    if (len == 0) {
        return 0;
    }

    uint32_t average = 0;

    for (uint8_t i = 0; i < len; i++) {
        average += data[i];
    }

    return average / len;
}

uint32_t _pump_get_average_difference(int32_t* data, uint32_t len)
{
    if (len <= 1) {
        return 0;
    }

    int32_t average_difference = 0;

    for (uint8_t i = 0; i < len - 1; i++) {
        average_difference += __abs_dif(data[i], data[i + 1]);
    }

    return average_difference / len;
}

bool _is_pump_not_working()
{
	uint32_t md212_measure_diffs = _pump_get_average_difference(pump_state.md212_measure_buf, __arr_len(pump_state.md212_measure_buf));
	return md212_measure_diffs < PUMP_MD212_CYCLE_INACCURACY;
}

bool _is_pump_and_valve_enabled()
{
	uint32_t pump_average  = _pump_get_average(pump_state.pump_measure_buf, __arr_len(pump_state.pump_measure_buf));
//	uint32_t valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));

	pump_average = PUMP_ADC_PUMP_MIN; // TODO: only for tests
	if (pump_average < PUMP_ADC_PUMP_MIN) {
		return false;
	}
//	if (valve_average < PUMP_ADC_VALVE_MIN) {
//		return false;
//	}

	return true;
}

bool _is_pump_and_valve_operable()
{
	uint32_t pump_average  = _pump_get_average(pump_state.pump_measure_buf, __arr_len(pump_state.pump_measure_buf));
//	uint32_t valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));

	if (pump_average > PUMP_ADC_PUMP_MAX) {
		return false;
	}
//	if (valve_average > PUMP_ADC_VALVE_MAX) {
//		return false;
//	}

	return true;
}

int32_t _get_pump_encoder_current_ticks()
{
	uint32_t value = __HAL_TIM_GET_COUNTER(&MD212_TIM);
	if (value >= PUMP_ENCODER_MIDDLE) {
		return (int32_t)(value - PUMP_ENCODER_MIDDLE);
	}
	return -((int32_t)(PUMP_ENCODER_MIDDLE - value));
}

void _reset_pump_encoder()
{
	__HAL_TIM_SET_COUNTER(&MD212_TIM, PUMP_ENCODER_MIDDLE);
}
