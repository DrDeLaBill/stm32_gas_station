/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _INDICATION_MANAGER_H_
#define _INDICATION_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void indicate_set_buffer(uint8_t* data, uint8_t len);
void indicate_proccess();
void indicate_set_load_page(bool enable);
void indicate_set_error_page(bool enable);


#ifdef __cplusplus
}
#endif

#endif
