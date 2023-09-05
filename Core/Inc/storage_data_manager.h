/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef INC_STORAGE_DATA_MANAGER_H_
#define INC_STORAGE_DATA_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include <eeprom_at24cm01_storage.h>


#define STORAGE_DEBUG (true)


typedef enum _storage_status_t {
    STORAGE_OK                  = ((uint8_t)0x00),
    STORAGE_ERROR               = ((uint8_t)0x01),
    STORAGE_ERROR_CRC           = ((uint8_t)0x02),
    STORAGE_ERROR_BLOCKED       = ((uint8_t)0x03),
    STORAGE_ERROR_BADACODE      = ((uint8_t)0x04),
    STORAGE_ERROR_VER           = ((uint8_t)0x05),
    STORAGE_ERROR_APPOINTMENT   = ((uint8_t)0x06),
    STORAGE_ERROR_OUT_OF_MEMORY = ((uint8_t)0x08),
    STORAGE_ERROR_BUSY          = ((uint8_t)0x09),
    STORAGE_ERROR_ADDR          = ((uint8_t)0x0A),
	STORAGE_ERROR_EMPTY         = ((uint8_t)0x0B)
} storage_status_t;

typedef enum _storage_header_status_appointment_t {
    STORAGE_HEADER_STATUS_APPOINTMENT_EMPTY            = ((uint8_t)0b00000000),
    STORAGE_HEADER_STATUS_APPOINTMENT_ERRORS_LIST_PAGE = ((uint8_t)0b00010000),
    STORAGE_HEADER_STATUS_APPOINTMENT_COMMON           = ((uint8_t)0b00100000)
} storage_header_status_appointment_t;

typedef enum _storage_header_status_t {
    STORAGE_HEADER_STATUS_EMPTY = ((uint8_t)0b00000000),
    STORAGE_HEADER_STATUS_START = ((uint8_t)0b00000001),
    STORAGE_HEADER_STATUS_END   = ((uint8_t)0b00000010)
} storage_header_status_t;


typedef struct __attribute__((packed)) _storage_payload_header_t {
    uint32_t magic;
    uint8_t  version;
    uint8_t  status;
    uint32_t next_addr;
} storage_payload_header_t;


#define STORAGE_PAYLOAD_MAGIC     ((uint32_t)(0xBEDAC0DE))
#define STORAGE_PAYLOAD_VERSION   ((uint8_t)(3))
#define STORAGE_PAGE_SIZE         (EEPROM_PAGE_SIZE)
#define STORAGE_PAGES_COUNT       (EEPROM_PAGES_COUNT)
#define STORAGE_START_ADDR        ((uint32_t)(0))
#define STORAGE_PAYLOAD_SIZE      (STORAGE_PAGE_SIZE - sizeof(struct _storage_payload_header_t) - sizeof(uint16_t))


typedef struct __attribute__((packed)) _storage_page_record_t {
    struct _storage_payload_header_t header;
    uint8_t payload[STORAGE_PAYLOAD_SIZE];
    uint16_t crc;
} storage_page_record_t;

#define STORAGE_PAYLOAD_ERRORS_SIZE (STORAGE_PAYLOAD_SIZE - sizeof(uint32_t))
typedef struct __attribute__((packed)) _storage_errors_list_page_t {
    uint32_t errors_list_page_num;
    uint8_t  blocked[STORAGE_PAYLOAD_ERRORS_SIZE];
} storage_errors_list_page_t;

typedef struct _storage_info_t {
    bool     is_errors_list_recieved;
    bool     is_errors_list_updated;
    uint32_t errors_addr_end;
} storage_info_t;

storage_status_t storage_load(uint32_t addr, uint8_t* data, uint32_t len);
storage_status_t storage_save(uint32_t addr, uint8_t* data, uint32_t len);
storage_status_t storage_get_first_available_addr(uint32_t* addr);
storage_status_t storage_get_next_available_addr(uint32_t prev_addr, uint32_t* next_addr);
storage_status_t storage_reset_errors_list_page();


#ifdef __cplusplus
}
#endif


#endif
