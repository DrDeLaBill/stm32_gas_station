/* Copyright © 2023 Georgy E. All rights reserved. */

#include "access_manager.h"

#include <stdint.h>
#include <stdbool.h>

#include "utils.h"
#include "umka200_manager.h"
#include "settings_manager.h"


#define ACCESS_TIMEOUT_MS ((uint32_t)30000)


typedef struct _access_state_t {
	util_timer_t wait_timer;
} access_state_t;


access_state_t access_state = {
	.wait_timer = { 0 }
};


void access_proccess()
{
	uint32_t user_card = umka200_get_rfid();

	if (device_info.access_granted && !util_is_timer_wait(&access_state.wait_timer)) {
		device_info.access_granted = false;
	}

	if (!user_card) {
		return;
	}

	for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
		if (settings.cards[i] == user_card) {
			device_info.access_granted = true;
			device_info.user_card      = user_card;
			util_timer_start(&access_state.wait_timer, ACCESS_TIMEOUT_MS);
			break;
		}
	}
}
