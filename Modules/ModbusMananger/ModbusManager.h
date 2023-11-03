/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <memory>
#include <stdint.h>

#if defined(STM32F100xB) || \
    defined(STM32F100xE) || \
    defined(STM32F101x6) || \
    defined(STM32F101xB) || \
    defined(STM32F101xE) || \
    defined(STM32F101xG) || \
    defined(STM32F102x6) || \
    defined(STM32F102xB) || \
    defined(STM32F103x6) || \
    defined(STM32F103xB) || \
    defined(STM32F103xE) || \
    defined(STM32F103xG) || \
    defined(STM32F105xC) || \
    defined(STM32F107xC)
    #include "stm32f1xx_hal.h"
#elif defined(STM32F405xx) || \
    defined(STM32F415xx) || \
    defined(STM32F407xx) || \
    defined(STM32F417xx) || \
    defined(STM32F427xx) || \
    defined(STM32F437xx) || \
    defined(STM32F429xx) || \
    defined(STM32F439xx) || \
    defined(STM32F401xC) || \
    defined(STM32F401xE) || \
    defined(STM32F410Tx) || \
    defined(STM32F410Cx) || \
    defined(STM32F410Rx) || \
    defined(STM32F411xE) || \
    defined(STM32F446xx) || \
    defined(STM32F469xx) || \
    defined(STM32F479xx) || \
    defined(STM32F412Cx) || \
    defined(STM32F412Zx) || \
    defined(STM32F412Rx) || \
    defined(STM32F412Vx) || \
    defined(STM32F413xx) || \
    defined(STM32F423xx)
    #include "stm32f4xx_hal.h"
#else
    #error "Please select first the target STM32Fxxx device used in your application (in eeprom_at24cm01_storage.c file)"
#endif

#include "main.h"
#include "utils.h"
#include "modbus_rtu_slave.h"


#define MB_MANAGER_BEDUG (true)


class ModbusManager
{
private:
    static const char TAG[];

    static const uint8_t SLAVE_ID = GENERAL_MODBUS_SLAVE_ID;

    static UART_HandleTypeDef* huart;

#if MB_MANAGER_BEDUG
    static uint16_t counter;
    static uint8_t request[20];

    static void showLogLine();
#endif
    static uint16_t data_length;
    static std::unique_ptr<uint8_t[]> data;
    static util_timer_t timer;

    static bool recievedNewData;
    static bool requestInProgress;


    static void response_data_handler(uint8_t* data, uint32_t len);
    static void request_error_handler();
    static void send_data();

    static void reset();

    static bool isWriteCommand(uint8_t command);

    void loadData();
    void updateData();

public:
    ModbusManager(UART_HandleTypeDef* huart);

    void tick();
    void recieveByte(uint8_t byte);
};


#endif
