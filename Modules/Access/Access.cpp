/* Copyright © 2023 Georgy E. All rights reserved. */

#include "Access.h"

#include <string.h>
#include <stdint.h>

#include "glog.h"
#include "gutils.h"
#include "wiegand.h"
#include "settings.h"


#define ACCESS_TIMEOUT_MS ((uint32_t)30000)


extern settings_t settings;

const char Access::TAG[] = "ACS";

utl::Timer Access::timer(ACCESS_TIMEOUT_MS);
uint32_t Access::card = 0;
bool Access::granted = false;
bool Access::denied = false;


void Access::check()
{
    uint32_t user_card = 0;

    if (wiegand_available()) {
        user_card = wiegant_get_value();
    }

    if (Access::granted && !timer.wait()) {
    	printTagLog(Access::TAG, "Access closed");
        Access::granted = false;
    }

    if (!user_card) {
        return;
    }

    if (Access::granted) {
    	return;
    }

    printTagLog(Access::TAG, "Card: %lu", user_card);
    if (user_card == SETTINGS_MASTER_CARD) {
        Access::denied  = false;
        Access::granted = true;
        Access::card    = user_card;
        timer.start();
        printTagLog(Access::TAG, "Access granted");
        return;
    }
    for (uint16_t i = 0; i < __arr_len(settings.cards); i++) {
        if (settings.cards[i] == user_card) {
            Access::denied  = false;
            Access::granted = true;
            Access::card    = user_card;
            timer.start();
            printTagLog(Access::TAG, "Access granted");
            return;
        }
    }
    Access::denied  = true;
    Access::granted = false;
    printTagLog(Access::TAG, "Access denied");
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
	printTagLog(Access::TAG, "Access closed");
    Access::granted = false;
    Access::denied = false;
    Access::card = 0;
    wiegand_reset();
}
