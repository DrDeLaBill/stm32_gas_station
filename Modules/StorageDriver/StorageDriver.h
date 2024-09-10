/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "Timer.h"
#include "StorageAT.h"


#define STORAGE_DRIVER_BEDUG      (true)
#define STORAGE_DRIVER_USE_BUFFER (true)


struct StorageDriver: public IStorageDriver
{
private:
	static constexpr char TAG[] = "DRVR";

	static bool hasError;
	static utl::Timer timer;

#if STORAGE_DRIVER_USE_BUFFER
    static bool     hasBuffer;
    static uint8_t  bufferPage[STORAGE_PAGE_SIZE];
    static uint32_t lastAddress;
#endif

public:
    StorageStatus read(const uint32_t address, uint8_t *data, const uint32_t len) override;
    StorageStatus write(const uint32_t address, const uint8_t *data, const uint32_t len) override;
    StorageStatus erase(const uint32_t*, const uint32_t) override;
};
