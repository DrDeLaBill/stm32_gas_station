/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _MODBUS_MANAGER_H_
#define _MODBUS_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void modbus_manager_init();
void modbus_manager_proccess();
void modbus_manager_recieve_data_byte(uint8_t byte);


#ifdef __cplusplus
}
#endif


#endif
