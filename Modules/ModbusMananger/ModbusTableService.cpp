/* Copyright © 2023 Georgy E. All rights reserved. */

#include "ModbusTableService.h"

#include <memory>
#include <stdint.h>

#include <modbus_rtu_slave.h>


std::shared_ptr<uint16_t[]> ModbusTableService::getRegisters(register_type_t register_type, uint32_t id, uint32_t reg_count)
{
    std::shared_ptr<uint16_t[]> regs(new uint16_t(reg_count), [] (uint16_t *arr) {
        delete [] arr;
    });
    for (unsigned i = 0; i < reg_count; i++) {
        regs[i] = modbus_slave_get_register_value(register_type, id + i);
    }
    return regs;
}

void ModbusTableService::setRegisters(register_type_t register_type, uint32_t id, std::shared_ptr<uint16_t[]> regs, uint8_t reg_count)
{
    for (unsigned i = 0; i < reg_count; i++) {
        modbus_slave_set_register_value(register_type, id + i, regs[i]);
    }
}
