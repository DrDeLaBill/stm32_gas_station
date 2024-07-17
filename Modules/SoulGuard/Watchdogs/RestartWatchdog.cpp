/* Copyright © 2024 Georgy E. All rights reserved. */

#include "Watchdogs.h"

#include "glog.h"
#include "main.h"
#include "system.h"
#include "hal_defs.h"

#include "UI.h"
#include "CodeStopwatch.h"


bool RestartWatchdog::flagsCleared = false;


void RestartWatchdog::check()
{
	utl::CodeStopwatch stopwatch(TAG, GENERAL_TIMEOUT_MS);

	if (flagsCleared) {
		return;
	}

	bool flag = false;
	// IWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
		printTagLog(TAG, "IWDG just went off");
		flag = true;
	}

	// WWDG check reboot
	if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
		printTagLog(TAG, "WWDG just went off");
		flag = true;
	}

	if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
		printTagLog(TAG, "SOFT RESET");
		flag = true;
	}

	if (flag) {
		__HAL_RCC_CLEAR_RESET_FLAGS();
		printTagLog(TAG, "DEVICE HAS BEEN REBOOTED");
		UI::setReboot();
		system_reset_i2c_errata();
		HAL_Delay(2500);
	}
}
