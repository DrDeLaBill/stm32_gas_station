/* Copyright © 2024 Georgy E. All rights reserved. */

#ifndef _RECORD_TYPE_H_
#define _RECORD_TYPE_H_


#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"


#define RECORD_BEDUG   (true)


#define RECORD_VERSION ((uint8_t)0x03)


typedef enum _RecordStatus {
    RECORD_OK = 0,
    RECORD_ERROR,
    RECORD_NO_LOG
} RecordStatus;

typedef struct __attribute__((packed)) _reocrd_t {
    uint32_t id;       // Record ID
    uint32_t time;     // Record time
    uint32_t card;     // User card ID
    uint32_t used_mls; // Session used liters
} record_t;

static const unsigned RECORD_SIZE = sizeof(record_t);


#ifdef __cplusplus
}
#endif

#endif
