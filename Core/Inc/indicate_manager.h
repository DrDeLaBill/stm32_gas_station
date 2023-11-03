/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _INDICATE_MANAGER_H_
#define _INDICATE_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


void indicate_set_buffer(uint8_t* data, uint8_t len);
void indicate_clear_buffer();
void indicate_proccess();
void indicate_display();
void indicate_set_wait_page();
void indicate_set_buffer_page();
void indicate_set_load_page();
void indicate_set_error_page();


#ifdef __cplusplus
}
#endif

#endif
