/* Copyright © 2024 Georgy E. All rights reserved. */

#include <log.h>
#include "StackWatchdog.h"

#include <cstring>

#include "main.h"
#include "soul.h"
#include "main.h"
#include "utils.h"
#include "bmacro.h"

#include "CodeStopwatch.h"


#define STACK_CANARY_WORD (0xBEDAC0DE)


unsigned StackWatchdog::lastFree = 0;


void STACK_WATCHDOG_FILL_RAM(void) {
	extern unsigned _ebss;
	volatile unsigned *top, *start;
	__asm__ volatile ("mov %[top], sp" : [top] "=r" (top) : : );
	start = &_ebss;
	while (start < top) {
		*(start++) = STACK_CANARY_WORD;
	}
}


void StackWatchdog::check()
{
	utl::CodeStopwatch stopwatch("STCK", GENERAL_TIMEOUT_MS);
	extern unsigned _ebss;
	unsigned *start, *end;
	__asm__ volatile ("mov %[end], sp" : [end] "=r" (end) : : );
	start = &_ebss;

	unsigned heap_end = 0;
	unsigned stack_end = 0;
	unsigned last_counter = 0;
	unsigned cur_counter = 0;
	for (;start < end; start++) {
		if ((*start) == STACK_CANARY_WORD) {
			cur_counter++;
		}
		if (cur_counter && (*start) != STACK_CANARY_WORD) {
			if (last_counter < cur_counter) {
				last_counter = cur_counter;
				heap_end     = (unsigned)start - cur_counter;
				stack_end    = (unsigned)start;
			}

			cur_counter = 0;
		}
	}

	if (!last_counter) {
		// TODO: an error is possible
	} else if (__abs_dif(lastFree, last_counter)) {
		extern unsigned _sdata;
		extern unsigned _estack;
		printTagLog(TAG, "-----ATTENTION! INDIRECT DATA BEGIN:-----");
		printTagLog(TAG, "RAM occupied MAX: %u bytes", __abs_dif((unsigned)&_sdata, (unsigned)&_estack) - last_counter);
		printTagLog(TAG, "RAM free  MIN:    %u bytes", last_counter);
		printTagLog(TAG, "------ATTENTION! INDIRECT DATA END-------");
	}

	if (last_counter) {
		lastFree = last_counter;
	}

	BEDUG_ASSERT(
		last_counter && lastFree && heap_end < stack_end,
		"STACK OVERFLOW IS POSSIBLE or you didn't use the function STACK_WATCHDOG_FILL_RAM on startup"
	);
}
