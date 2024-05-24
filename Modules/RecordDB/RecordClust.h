/* Copyright © 2024 Georgy E. All rights reserved. */

#pragma once


#include <stdint.h>

#include "main.h"
#include "bmacro.h"

#include "StorageAT.h"
#include "RecordType.h"


class RecordClust
{
public:
    static constexpr char PREFIX[] = "RDC";

    static constexpr unsigned META_SIZE    = sizeof(uint8_t) + sizeof(uint16_t);
    static constexpr unsigned RECORDS_SIZE = STORAGE_PAGE_PAYLOAD_SIZE - META_SIZE;
    typedef struct __attribute__((packed)) _record_clust_t {
        // Software version
        uint8_t  version;
        // Record structure unit size in cluster
        uint16_t rcrd_size;
        // Buffer for record/s
        uint8_t  records[RECORDS_SIZE];

        record_t& operator[](unsigned i);

        uint32_t count();
        uint32_t size();
    } record_clust_t;

protected:
    static constexpr char TAG[] = "RDC";

    uint32_t m_recordId;
    uint16_t m_recordSize;
    uint32_t m_address;
    record_clust_t m_clust;

    bool validate(record_clust_t* clust);
    bool loadExist(bool validateSize);
    bool createNew();
    RecordStatus getMaxId(uint32_t *maxId);
    static uint32_t getCountByRecordSize(uint32_t recordSize);

public:
    RecordClust(uint32_t recordId = 0, uint16_t recordSize = 0);

    record_t& operator[](unsigned i);

    RecordStatus load(bool validateSize = true);
    RecordStatus save(record_t *record, uint32_t size);
    // TODO: RecordStatus erase(uint32_t address); with w25xx.h _flash_erase_data

    uint32_t records_count();
    uint32_t structure_size();
    void show();
    void showMax();

private:
    RecordStatus deleteClust(uint32_t address);

};
