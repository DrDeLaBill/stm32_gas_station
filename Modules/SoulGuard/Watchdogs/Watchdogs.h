/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include "Timer.h"
#include "FiniteStateMachine.h"


#define SETTINGS_WATCHDOG_BEDUG (true)
#define POWER_WATCHDOG_BEDUG    (true)


#define WATCHDOG_TIMEOUT_MS     ((uint32_t)100)


/*
 * Filling an empty area of RAM with the STACK_CANARY_WORD value
 * For calculating the RAM fill factor
 */
extern "C" void STACK_WATCHDOG_FILL_RAM(void);


struct StackWatchdog
{
	void check();

private:
	static constexpr char TAG[] = "STCK";
	static unsigned lastFree;
	static utl::Timer timer;
};

struct RestartWatchdog
{
public:
	// TODO: check IWDG or another reboot
	void check();

	static void reset_i2c_errata();

private:
	static constexpr char TAG[] = "RSTw";
	static bool flagsCleared;

};

struct RTCWatchdog
{
	static constexpr char TAG[] = "RTCw";

	void check();
};


struct SettingsWatchdog
{
	SettingsWatchdog();

	void check();
};


struct PowerWatchdog
{
	void check();
};

struct MemoryWatchdog
{
private:
	static constexpr uint32_t TIMEOUT_MS = 15000;

	utl::Timer errorTimer;
	utl::Timer timer;
	uint8_t errors;
	bool timerStarted;

public:
	MemoryWatchdog();

	void check();
};
