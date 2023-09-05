/* Copyright © 2023 Georgy E. All rights reserved. */

#include "pump_manager.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "main.h"
#include "utils.h"


#define PUMP_MD212_CYCLE_IMPULSES_COUNT ((uint32_t)200)
#define PUMP_MD212_CYCLE_ML_VALUE       ((uint32_t)1000)
#define PUMP_MD212_CYCLE_INACCURACY     ((uint32_t)20)
#define PUMP_MD212_MEASURE_DELAY_MS     ((uint32_t)10)

#define PUMP_ADC_READ_TIMEOUT_MS        ((uint32_t)100)
#define PUMP_ADC_MEASURE_DELAY_MS       ((uint32_t)10)
#define PUMP_ADC_OVERHEAD               ((uint32_t)4000)
#define PUMP_ADC_VALVE_MIN              ((uint32_t)2500)
#define PUMP_ADC_PUMP_MIN               ((uint32_t)2500)

#define PUMP_SESSION_ML_MIN             ((uint32_t)1000)
#define PUMP_CHECK_START_DELAY_MS       ((uint32_t)2000)
#define PUMP_CHECK_STOP_DELAY_MS        ((uint32_t)2000)
#define PUMP_MEASURE_BUFFER_SIZE        ((uint8_t)10)

#define PUMP_SLOW_ML_VALUE              ((uint32_t)5000)


typedef struct _pump_state_t {
    uint32_t     target_ml;
    void         (*fsm_pump_state) (void);
    void         (*pump_stop_handler) (void);
    bool         pump_error;

    uint32_t     md212_counter;
    uint32_t     md212_measure_buf[PUMP_MEASURE_BUFFER_SIZE];

    uint32_t     pump_counter;
    uint32_t     valve_measure_buf[PUMP_MEASURE_BUFFER_SIZE];
    uint32_t     pump_measure_buf [PUMP_MEASURE_BUFFER_SIZE];

    util_timer_t wait_timer;
    util_timer_t error_timer;
} pump_state_t;


void _pump_fsm_wait();
void _pump_fsm_start();
void _pump_fsm_check_start();
void _pump_fsm_count_ml();
void _pump_fsm_stop();
void _pump_fsm_check_stop();
void _pump_fsm_error();

void _pump_set_pump_enable(GPIO_PinState enable_state);
void _pump_set_valve1_enable(GPIO_PinState enable_state);
void _pump_set_valve2_enable(GPIO_PinState enable_state);
void _pump_reset_state();

uint32_t _pump_get_adc_valve_current();
uint32_t _pump_get_adc_pump_current();
uint32_t _pump_get_average(uint32_t* data, uint32_t len);
uint32_t _pump_get_average_difference(uint32_t* data, uint32_t len);


pump_state_t pump_state = {
    .target_ml         = 0,
    .fsm_pump_state    = _pump_fsm_stop,
    .pump_stop_handler = NULL,
    .pump_error        = false,

    .md212_counter     = 0,
    .md212_measure_buf = { 0 },

    .pump_counter      = 0,
    .pump_measure_buf  = { 0 },
    .valve_measure_buf = { 0 },

    .wait_timer        = { 0 },
    .error_timer       = { 0 },
};


void pump_set_fuel(uint32_t amount_ml)
{
    if (pump_state.pump_error) {
        return;
    }
    if (amount_ml > GENERAL_SESSION_ML_MAX) {
        amount_ml = GENERAL_SESSION_ML_MAX;
    }
    if (amount_ml < PUMP_SESSION_ML_MIN) {
        return;
    }
    pump_state.target_ml = amount_ml;
}

void pump_proccess()
{
    pump_state.fsm_pump_state();
}

uint32_t pump_get_fuel_count_ml()
{
    return __HAL_TIM_GET_COUNTER(&MD212_TIM);
}

bool pump_has_error()
{
    return pump_state.pump_error;
}

void pump_set_pump_stop_handler(void (*pump_stop_handler) (void))
{
    if (pump_stop_handler != NULL) {

    }
}

void _pump_fsm_wait()
{
    if (pump_state.pump_error) {
        pump_state.fsm_pump_state = &_pump_fsm_error;
        return;
    }

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    uint32_t md212_wait_inaccuracy = 0;
    if (pump_state.md212_counter < __arr_len(pump_state.md212_measure_buf)) {
        util_timer_start(&pump_state.wait_timer, PUMP_MD212_MEASURE_DELAY_MS);
        pump_state.md212_measure_buf[pump_state.md212_counter++] = __HAL_TIM_GET_COUNTER(&MD212_TIM);
        return;
    } else {
        md212_wait_inaccuracy = _pump_get_average_difference(pump_state.md212_measure_buf, __arr_len(pump_state.md212_measure_buf));
    }

    if (md212_wait_inaccuracy > PUMP_MD212_CYCLE_INACCURACY) {
        pump_state.pump_error = true;
        return;
    }

    if (pump_state.target_ml) {
        pump_state.fsm_pump_state = &_pump_fsm_start;
    }
}

void _pump_fsm_start()
{
    __HAL_TIM_SET_COUNTER(&MD212_TIM, 0);

    _pump_set_pump_enable(GPIO_PIN_SET);
    _pump_set_valve1_enable(GPIO_PIN_SET);
    _pump_set_valve2_enable(GPIO_PIN_SET);

    util_timer_start(&pump_state.error_timer, PUMP_CHECK_START_DELAY_MS);
    util_timer_start(&pump_state.wait_timer, 0);
    pump_state.fsm_pump_state = &_pump_fsm_check_start;
}

void _pump_fsm_check_start()
{
    if (!util_is_timer_wait(&pump_state.error_timer)) {
        pump_state.pump_error     = true;
        pump_state.fsm_pump_state = &_pump_fsm_stop;
        return;
    }

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }


    uint32_t pump_average   = 0;
    uint32_t valve_average = 0;
    if (pump_state.pump_counter < __arr_len(pump_state.pump_measure_buf)) {
        util_timer_start(&pump_state.wait_timer, PUMP_ADC_MEASURE_DELAY_MS);
        pump_state.pump_measure_buf[pump_state.pump_counter] = _pump_get_adc_pump_current();
        pump_state.valve_measure_buf[pump_state.pump_counter++] = _pump_get_adc_valve_current();
        return;
    } else {
        pump_average  = _pump_get_average(pump_state.pump_measure_buf, __arr_len(pump_state.pump_measure_buf));
        valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));
    }

    bool pump_statred = true;
    if (pump_average < PUMP_ADC_PUMP_MIN) {
        pump_statred = false;
    }
    if (valve_average < PUMP_ADC_VALVE_MIN) {
        pump_statred = false;
    }

    util_timer_start(&pump_state.error_timer, 0);
    util_timer_start(&pump_state.wait_timer, 0);
    if (pump_statred) {
        pump_state.fsm_pump_state = &_pump_fsm_count_ml;
    } else {
        pump_state.pump_error = true;
        pump_state.fsm_pump_state = &_pump_fsm_stop;
    }
}

void _pump_fsm_count_ml()
{
    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    util_timer_start(&pump_state.wait_timer, PUMP_MD212_MEASURE_DELAY_MS);
    pump_state.md212_measure_buf[pump_state.md212_counter++] = __HAL_TIM_GET_COUNTER(&MD212_TIM);

    if (pump_state.md212_counter < __arr_len(pump_state.md212_measure_buf)) {
        return;
    }

    uint32_t md212_measure_diffs = _pump_get_average_difference(pump_state.md212_measure_buf, __arr_len(pump_state.md212_measure_buf));
    if (md212_measure_diffs < PUMP_MD212_CYCLE_INACCURACY) {
        pump_state.pump_error = true;
        pump_state.fsm_pump_state = &_pump_fsm_stop;
        return;
    }

    if (pump_state.target_ml <= PUMP_SLOW_ML_VALUE) {
        _pump_set_valve1_enable(GPIO_PIN_RESET);
    }

    uint32_t ml_current_count     = (__HAL_TIM_GET_COUNTER(&MD212_TIM) * PUMP_MD212_CYCLE_ML_VALUE) / PUMP_MD212_CYCLE_IMPULSES_COUNT;
    uint32_t ml_fast_count_target = pump_state.target_ml - PUMP_SLOW_ML_VALUE;
    if (ml_current_count >= ml_fast_count_target) {
        _pump_set_valve1_enable(GPIO_PIN_RESET);
    }

    if (ml_current_count >= pump_state.target_ml) {
        util_timer_start(&pump_state.error_timer, 0);
        util_timer_start(&pump_state.wait_timer, 0);
        pump_state.fsm_pump_state = &_pump_fsm_stop;
    }
}

void _pump_fsm_stop()
{
    _pump_set_pump_enable(GPIO_PIN_RESET);
    _pump_set_valve1_enable(GPIO_PIN_RESET);
    _pump_set_valve2_enable(GPIO_PIN_RESET);

    util_timer_start(&pump_state.error_timer, PUMP_CHECK_STOP_DELAY_MS);
    util_timer_start(&pump_state.wait_timer, 0);
    pump_state.fsm_pump_state = &_pump_fsm_check_stop;
}

void _pump_fsm_check_stop()
{
    if (!util_is_timer_wait(&pump_state.error_timer)) {
        pump_state.pump_error     = true;
        pump_state.fsm_pump_state = &_pump_fsm_error;
        return;
    }

    if (util_is_timer_wait(&pump_state.wait_timer)) {
        return;
    }

    uint32_t pump_average   = 0;
    uint32_t valve_average = 0;
    if (pump_state.pump_counter < __arr_len(pump_state.pump_measure_buf)) {
        util_timer_start(&pump_state.wait_timer, PUMP_ADC_MEASURE_DELAY_MS);
        pump_state.pump_measure_buf[pump_state.pump_counter] = _pump_get_adc_pump_current();
        pump_state.valve_measure_buf[pump_state.pump_counter++] = _pump_get_adc_valve_current();
        return;
    } else {
        pump_average  = _pump_get_average(pump_state.pump_measure_buf, __arr_len(pump_state.pump_measure_buf));
        valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));
    }

    bool pump_stopped = true;
    if (pump_average > PUMP_ADC_PUMP_MIN) {
        pump_stopped = false;
    }
    if (valve_average > PUMP_ADC_VALVE_MIN) {
        pump_stopped = false;
    }

    util_timer_start(&pump_state.error_timer, 0);
    util_timer_start(&pump_state.wait_timer, 0);
    if (pump_stopped) {
        pump_state.fsm_pump_state = &_pump_fsm_wait;
    } else {
        pump_state.pump_error = true;
        pump_state.fsm_pump_state = &_pump_fsm_error;
    }
}

void _pump_fsm_error()
{
    pump_state.pump_error        = true;
    pump_state.pump_stop_handler = NULL;

    _pump_set_pump_enable(GPIO_PIN_RESET);
    _pump_set_valve1_enable(GPIO_PIN_RESET);
    _pump_set_valve2_enable(GPIO_PIN_RESET);

    __HAL_TIM_SET_COUNTER(&MD212_TIM, 0);
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
    pump_stop_handler = pump_state.pump_stop_handler;

    memset((uint8_t*)&pump_state, 0, sizeof(pump_state));

    pump_state.fsm_pump_state = &_pump_fsm_stop;
    pump_state.pump_stop_handler = pump_stop_handler;

    __HAL_TIM_SET_COUNTER(&MD212_TIM, 0);
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

uint32_t _pump_get_average_difference(uint32_t* data, uint32_t len)
{
    if (len <= 1) {
        return 0;
    }

    uint32_t average_difference = 0;

    for (uint8_t i = 0; i < len - 1; i++) {
        average_difference += __abs_dif(data[i], data[i + 1]);
    }

    return average_difference / len;
}
