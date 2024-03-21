#ifndef _TM1637_H_
#define _TM1637_H_


#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include <stdbool.h>

#include "hal_defs.h"


#define NUM_OF_DIGITS         (6)   // количество разрядов
#define NUM_OF_SEGMENTS       (8)

#define CMD_WR_DATA           0x40   // команда записи данных в дисплей с автоинкрементом
#define CMD_SET_ADDR          0xC0   // команда установки разряда
#define CMD_DISPLAY           0x80   // команада вывода изображения

#define CMD_SET_ADDR0         0xC0   // 1 разряд
#define CMD_SET_ADDR1         0xC1   // 2 разряд
#define CMD_SET_ADDR2         0xC2   // 3 разряд
#define CMD_SET_ADDR3         0xC3   // 4 разряд
#define CMD_SET_ADDR4         0xC4   // 5 разряд
#define CMD_SET_ADDR5         0xC5   // 6 разряд

#define CMD_DISPLAY_1_16      0x88    // 
#define CMD_DISPLAY_2_16      0x89    //
#define CMD_DISPLAY_4_16      0x8A    // 
#define CMD_DISPLAY_10_16     0x8B    //
#define CMD_DISPLAY_11_16     0x8C    // 
#define CMD_DISPLAY_12_16     0x8D    //
#define CMD_DISPLAY_13_16     0x8E    // 
#define CMD_DISPLAY_14_16     0x8F    //

#define CMD_DISPLAY_OFF       0x80    // turn the display off

#define DOT_ENABLE            0x04

#define S_MINUS               10
#define S_SPACE               11
#define S_CLEAR               12


void tm1637_init(GPIO_PAIR* clk_ptr, GPIO_PAIR* data_ptr);
void tm1637_proccess();
void tm1637_set_buffer(uint8_t data[NUM_OF_DIGITS]);
void tm1637_set_dot(uint8_t index, bool enable);


#ifdef __cplusplus
}
#endif


#endif
