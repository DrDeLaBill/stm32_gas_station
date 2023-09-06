/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _RECORD_MANAGER_H_
#define _RECORD_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "storage_data_manager.h"


#define RECORD_DEBUG    (true)
#define RECORD_FIRST_ID ((uint32_t)1)


typedef enum _record_status_t {
	RECORD_OK = 0,
	RECORD_ERROR,
	RECORD_NO_LOG
} record_status_t;


#define RECORD_TIME_ARRAY_SIZE 6
typedef struct __attribute__((packed)) _log_record_t {
	uint32_t id;                           // Record ID
	uint8_t  time[RECORD_TIME_ARRAY_SIZE]; // Record time
	uint32_t cf_id;                        // Configuration version
	uint32_t card;                         // User card ID
	uint32_t used_liters;                  // Session used liters
} log_record_t;


#define RECORDS_CLUST_SIZE  ((STORAGE_PAYLOAD_SIZE - sizeof(uint8_t)) / sizeof(struct _log_record_t))
#define RECORDS_CLUST_MAGIC (sizeof(struct _log_record_t))

typedef struct __attribute__((packed)) _log_record_clust_t {
	uint8_t      record_magic;
	log_record_t records[RECORDS_CLUST_SIZE];
}log_record_clust_t;

typedef struct _log_ids_cache_t {
	bool is_need_to_scan;
	uint32_t cur_scan_address;
	uint32_t first_record_addr;
	uint32_t ids_cache[STORAGE_PAGES_COUNT][RECORDS_CLUST_SIZE];
} log_ids_cache_t;


extern log_record_t log_record;


record_status_t next_record_load();
record_status_t record_save();
record_status_t record_get_new_id();
record_status_t record_delete_record(uint32_t id);
void            record_cache_records_proccess();


#ifdef __cplusplus
}
#endif


#endif
