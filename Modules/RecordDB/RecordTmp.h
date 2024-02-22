/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _RECORD_TMP_H_
#define _RECORD_TMP_H_


#include <cstdint>

#include "RecordType.h"


#define RECORD_TMP_BEDUG (true)


struct RecordTmp
{
private:
	static constexpr char PREFIX[] = "RCT";
	static constexpr char TAG[] = "RCT";

	static uint32_t lastAddress;

	static RecordStatus findAddress();

public:
	static bool exists();
	static void init();
	static RecordStatus save(const uint32_t card, const uint32_t lastMl);
	static RecordStatus restore();
	static RecordStatus remove();
};

#endif
