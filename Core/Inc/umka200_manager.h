/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _UMKA200_MANAGER_H_
#define _UMKA200_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>


#define UMKA200_MESSAGE_META_SIZE (3 * sizeof(uint8_t))
#define UMKA200_MESSAGE_DATA_SIZE ((uint16_t)255 - UMKA200_MESSAGE_META_SIZE)


typedef enum _umka200_resposne_status_t {
	UMKA200_OK               = ((uint8_t)0x00),
	UMKA200_ERROR            = ((uint8_t)0x01),
	UMKA200_ERROR_ID         = ((uint8_t)0x02),
	UMKA200_ERROR_LENGTH     = ((uint8_t)0x03),
	UMKA200_ERROR_COMMAND    = ((uint8_t)0x04),
	UMKA200_ERROR_SUBCOMMAND = ((uint8_t)0x05),
	UMKA200_ERROR_STATUS     = ((uint8_t)0x06),
	UMKA200_ERROR_CRC        = ((uint8_t)0x07)
} umka200_resposne_status_t;


void     umka200_proccess();
uint32_t umka200_get_rfid();

void umka200_recieve_byte(uint8_t byte);

void umka200_set_data_sender(void (*data_sender_handler) (uint8_t* data, uint16_t len));
void umka200_timeout();


#ifdef __cplusplus
}
#endif


#endif
