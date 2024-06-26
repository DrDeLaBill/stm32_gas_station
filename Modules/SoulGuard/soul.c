/* Copyright © 2024 Georgy E. All rights reserved. */

#include "soul.h"

#include <stdint.h>
#include <stdbool.h>


static soul_t soul = {
	.last_err = 0,
	.statuses = { 0 }
};


bool _is_status(SOUL_STATUS status);
void _set_status(SOUL_STATUS status);
void _reset_status(SOUL_STATUS status);


unsigned get_last_error()
{
	return soul.last_err;
}

void set_last_error(SOUL_STATUS error)
{
	if (ERRORS_START < error && error < ERRORS_END) {
		soul.last_err = error;
	}
}

bool has_errors()
{
	for (unsigned i = ERRORS_START + 1; i < ERRORS_END; i++) {
		if (is_error((SOUL_STATUS)(i))) {
			return true;
		}
	}
	return false;
}

bool is_error(SOUL_STATUS error)
{
	if (error > ERRORS_START && error < ERRORS_END) {
		return _is_status(error);
	}
	return false;
}

void set_error(SOUL_STATUS error)
{
	if (error > ERRORS_START && error < ERRORS_END) {
		_set_status(error);
	}
}

unsigned get_first_error()
{
	for (unsigned i = ERRORS_START + 1; i < ERRORS_END; i++) {
		if (is_error((SOUL_STATUS)(i))) {
			return i;
		}
	}
	return 0;
}

void reset_error(SOUL_STATUS error)
{
	if (error > ERRORS_START && error < ERRORS_END) {
		_reset_status(error);
	}
}

bool is_status(SOUL_STATUS status)
{
	if (status > STATUSES_START && status < STATUSES_END) {
		return _is_status(status);
	}
	return false;
}

void set_status(SOUL_STATUS status)
{
	if (status > STATUSES_START && status < STATUSES_END) {
		_set_status(status);
	}
}

void reset_status(SOUL_STATUS status)
{
	if (status > STATUSES_START && status < STATUSES_END) {
		_reset_status(status);
	}
}

bool _is_status(SOUL_STATUS status)
{
	uint8_t status_num = (uint8_t)(status) - 1;
	return (bool)(
		(
			soul.statuses[status_num / BITS_IN_BYTE] >>
			(status_num % BITS_IN_BYTE)
		) & 0x01
	);
}

void _set_status(SOUL_STATUS status)
{
	if (status == 0) {
		return;
	}
	uint8_t status_num = (uint8_t)(status) - 1;
	soul.statuses[status_num / BITS_IN_BYTE] |= (0x01 << (status_num % BITS_IN_BYTE));
}

void _reset_status(SOUL_STATUS status)
{
	if (status == 0) {
		return;
	}
	uint8_t status_num = (uint8_t)(status) - 1;
	soul.statuses[status_num / BITS_IN_BYTE] &= (uint8_t)~(0x01 << (status_num % BITS_IN_BYTE));
}
