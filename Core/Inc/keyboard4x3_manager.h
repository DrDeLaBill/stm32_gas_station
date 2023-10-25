/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _KEYBOARD4X3_MANAGER_H_
#define _KEYBOARD4X3_MANAGER_H_


#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


#define KEYBOARD4X3_BUFFER_SIZE               ((uint8_t)6)
#define KEYBOARD4X3_COLS_COUNT                ((uint8_t)3)
#define KEYBOARD4X3_ROWS_COUNT                ((uint8_t)4)
#define KEYBOARD4X3_VALUE_POINT_SYMBOLS_COUNT ((uint32_t)1)


void     keyboard4x3_proccess();
uint8_t* keyboard4x3_get_buffer();


#ifdef __cplusplus
}
#endif

#endif
