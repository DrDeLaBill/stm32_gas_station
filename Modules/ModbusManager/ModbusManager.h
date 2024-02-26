/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <memory>
#include <stdint.h>

#include "main.h"
#include "utils.h"
#include "hal_defs.h"
#include "modbus_rtu_slave.h"

#include "Timer.h"


#define MB_MANAGER_BEDUG  (false)
#define MB_PROTOCOL_BEDUG (false)


class ModbusManager
{
private:
    static const char TAG[];

    static const uint8_t SLAVE_ID = GENERAL_MODBUS_SLAVE_ID;

    static UART_HandleTypeDef* huart;

#if MB_PROTOCOL_BEDUG
    static uint16_t counter;
    static uint8_t request[20];

#endif
    static uint16_t data_length;
    static std::unique_ptr<uint8_t[]> data;
    static utl::Timer timer;

    static bool recievedNewData;
    static bool requestInProgress;


    static void response_data_handler(uint8_t* data, uint32_t len);
    static void request_error_handler();
    static void send_data();
    static void reset();
    static void showLogLine();

    static bool isWriteCommand(uint8_t command);

    void loadData();
    void updateData();

public:
    ModbusManager(UART_HandleTypeDef* huart);

    void tick();
    void recieveByte(uint8_t byte);
};


#endif
