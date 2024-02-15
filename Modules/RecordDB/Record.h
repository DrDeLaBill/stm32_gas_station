/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef RECORD_DB_H
#define RECORD_DB_H


#include <stdint.h>

#include "StorageAT.h"
#include "RecordType.h"


#define RECORD_BEDUG (true)


class Record
{
public:
    record_t record;

    Record(uint32_t recordId);

    RecordStatus save();
    RecordStatus load();
    RecordStatus loadNext();

    void show();
    uint16_t size();

private:
    static constexpr char TAG[] = "RCR";
    static constexpr char PREFIX[] = "RCR";

    uint32_t m_recordId;
    uint16_t m_sensCount;
    uint8_t m_counter;

};


#endif
