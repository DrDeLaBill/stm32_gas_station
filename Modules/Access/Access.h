/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef ACCESS_H
#define ACCESS_H


#include <stdint.h>

#include "utils.h"


class Access
{
private:
    static const char TAG[];

    static util_timer_t timer;
    static uint32_t card;
    static bool granted;
    static bool denied;

    static void check();

public:
    static uint32_t getCard();
    static bool isGranted();
    static bool isDenied();
    static void close();
};


#endif
