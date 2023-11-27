/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef _WIEGAND_H_
#define _WIEGAND_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>


bool     wiegand_available();
uint32_t wiegant_get_value();
void     wiegand_reset();
void     wiegand_set_value(uint8_t value);


#ifdef __cplusplus
}
#endif


#endif
