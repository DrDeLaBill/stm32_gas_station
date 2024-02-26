/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include "SoulGuard.h"


extern SoulGuard<
	RestartWatchdog,
	MemoryWatchdog,
	StackWatchdog,
	SettingsWatchdog,
	PowerWatchdog,
	RTCWatchdog
> soulGuard;
