/* Copyright © 2023 Georgy E. All rights reserved. */

#include "Access.h"

#include <string.h>
#include <stdint.h>

#include "SettingsDB.h"

#include "utils.h"
#include "wiegand.h"


#define ACCESS_TIMEOUT_MS ((uint32_t)30000)


extern SettingsDB settings;

const char Access::TAG[] = "ACS";

util_timer_t Access::timer = { 0 };
uint32_t Access::card = 0;
bool Access::granted = false;
bool Access::denied = false;


void Access::check()
{
    uint32_t user_card = 0;

    if (wiegand_available()) {
        user_card = wiegant_get_value();
    }

    if (Access::granted && !util_is_timer_wait(&Access::timer)) {
        LOG_TAG_BEDUG(Access::TAG, "Access closed");
        Access::granted = false;
    }

    if (!user_card) {
        return;
    }

    LOG_TAG_BEDUG(Access::TAG, "Card: %lu", user_card);
    for (uint16_t i = 0; i < __arr_len(settings.settings.cards); i++) {
        if (settings.settings.cards[i] == user_card) {
            Access::granted = true;
            Access::card    = user_card;
            util_timer_start(&Access::timer, ACCESS_TIMEOUT_MS);
            LOG_TAG_BEDUG(Access::TAG, "Access granted");
            return;
        }
    }
    Access::denied = true;
    LOG_TAG_BEDUG(Access::TAG, "Access denied");
}

uint32_t Access::getCard()
{
	Access::check();
    return Access::card;
}

bool Access::isGranted()
{
	Access::check();
    return Access::granted;
}

bool Access::isDenied()
{
	Access::check();
	return Access::denied;
}

void Access::close()
{
    LOG_TAG_BEDUG(Access::TAG, "Access closed");
    Access::granted = false;
    Access::denied = false;
}
