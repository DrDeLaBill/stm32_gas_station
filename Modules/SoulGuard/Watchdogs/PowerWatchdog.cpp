/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "system.h"
#include "hal_defs.h"

#include "CodeStopwatch.h"


void PowerWatchdog::check()
{
	utl::CodeStopwatch stopwatch("PWRw", WATCHDOG_TIMEOUT_MS);

	if (!is_status(WORKING)) {
		return;
	}

	uint16_t voltage = 0;

	if (SYSTEM_ADC_VOLTAGE) {
		voltage = STM_ADC_MAX * STM_REF_VOLTAGEx10 / SYSTEM_ADC_VOLTAGE;
	}

	if (STM_MIN_VOLTAGEx10 <= voltage && voltage <= STM_MAX_VOLTAGEx10) {
		reset_error(POWER_ERROR);
	} else {
		set_error(POWER_ERROR);
	}
}
