/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "StorageAT.h"


#define STORAGE_DRIVER_BEDUG (false)


struct StorageDriver: public IStorageDriver
{
private:
	static constexpr char TAG[] = "DRVR";

    static bool    hasBuffer;
    static uint8_t bufferPage[STORAGE_PAGE_SIZE];
    static uint32_t lastAddress;

public:
    StorageStatus read(uint32_t address, uint8_t *data, uint32_t len) override;
    StorageStatus write(uint32_t address, uint8_t *data, uint32_t len) override;

};
