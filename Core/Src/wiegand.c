/* Copyright © 2023 Georgy E. All rights reserved. */

#include "wiegand.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "utils.h"


#define WIEGAND_MAX_BITS_COUNT ((uint8_t)26)
#define WIEGAND_STOP_MS        ((uint32_t)10)
#define WIEGAND_RESET_MS       ((uint32_t)500)
#define WIEGAND_VALUE_LEN      ((uint8_t)WIEGAND_MAX_BITS_COUNT - 2)


typedef struct _wiegand_state_t {
    util_timer_t stop_timer;
    util_timer_t reset_timer;
    uint8_t      bit_counter;
    bool         data[WIEGAND_MAX_BITS_COUNT];
    uint32_t     value;
} wiegand_state_t;

wiegand_state_t wiegand_state = {
    .reset_timer = { 0 },
    .reset_timer = { 0 },
    .bit_counter = 0,
    .data        = { 0 },
    .value       = 0
};


void _wiegand_parse_data();
void _wiegand_clear();


bool wiegand_available()
{
    return wiegand_state.bit_counter == WIEGAND_MAX_BITS_COUNT;
}

uint32_t wiegant_get_value()
{
    if (wiegand_state.bit_counter == WIEGAND_MAX_BITS_COUNT) {
        uint32_t tmp_value  = wiegand_state.value;
        _wiegand_clear();
        return tmp_value;
    }
    return 0;
}

void wiegand_set_value(uint8_t value)
{
    if (!util_is_timer_wait(&wiegand_state.reset_timer)) {
        _wiegand_clear();
    }

    if (wiegand_state.bit_counter == WIEGAND_MAX_BITS_COUNT) {
        return;
    }

    if (!util_is_timer_wait(&wiegand_state.stop_timer)) {
        _wiegand_clear();
    }

    wiegand_state.data[wiegand_state.bit_counter++] = (bool)value;

    util_timer_start(&wiegand_state.reset_timer, WIEGAND_RESET_MS);
    util_timer_start(&wiegand_state.stop_timer, WIEGAND_STOP_MS);

    if (wiegand_state.bit_counter == WIEGAND_MAX_BITS_COUNT) {
        _wiegand_parse_data();
    }
}

void _wiegand_parse_data()
{
    uint8_t counter = 0;

    bool control_bit_first = wiegand_state.data[counter++];

    uint8_t unit_counter = 0;
    uint16_t value = 0;
    for (uint8_t i = 0; i < WIEGAND_VALUE_LEN / 2; i++) {
        value       <<= 1;
        value        |= ((uint16_t)(wiegand_state.data[counter + i] ? 1 : 0));
        unit_counter += wiegand_state.data[counter + i];
    }
    counter += WIEGAND_VALUE_LEN / 2;

    if (!(((uint8_t)control_bit_first) ^ ((uint8_t)(unit_counter % 2 == 0)))) {
        _wiegand_clear();
        return;
    }

    wiegand_state.value = (((uint32_t)value) << WIEGAND_VALUE_LEN / 2);

    unit_counter = 0;
    value = 0;
    for (uint8_t i = 0; i < WIEGAND_VALUE_LEN / 2; i++) {
        value       <<= 1;
        value        |= ((uint16_t)(wiegand_state.data[counter + i] ? 1 : 0));
        unit_counter += wiegand_state.data[counter + i];
    }
    counter += WIEGAND_VALUE_LEN / 2;

    bool control_bit_last = wiegand_state.data[counter++];

    if (!(((uint8_t)control_bit_last) ^ ((uint8_t)(unit_counter % 2 == 1)))) {
        _wiegand_clear();
        return;
    }

    wiegand_state.value |= ((uint32_t)value);
}

void _wiegand_clear()
{
    memset((uint8_t*)&wiegand_state, 0, sizeof(wiegand_state));
}
