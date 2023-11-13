/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef MODBUS_TABLE_SERVICE_H
#define MODBUS_TABLE_SERVICE_H


#include <memory>
#include <stdint.h>

#include "modbus_rtu_slave.h"


class ModbusTableService
{
public:
    static std::shared_ptr<uint16_t[]> getRegisters(register_type_t register_type, uint32_t id, uint32_t reg_count);
    static void setRegisters(register_type_t register_type, uint32_t id, std::shared_ptr<uint16_t[]> regs, uint8_t reg_count);
};

#endif
