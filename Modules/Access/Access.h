/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef ACCESS_H
#define ACCESS_H


#include <stdint.h>

#include "utils.h"

#include "Timer.h"


class Access
{
private:
    static const char TAG[];

    static utl::Timer timer;
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
