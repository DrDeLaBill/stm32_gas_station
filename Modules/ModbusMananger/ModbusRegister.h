/* Copyright © 2023 Georgy E. All rights reserved. */

#ifndef MODBUS_REGISTER_H
#define MODBUS_REGISTER_H


#include <memory>
#include <string.h>
#include <stdint.h>

#include "ModbusTableService.h"

#include "utils.h"
#include "modbus_rtu_base.h"

#include "stm32f4xx_hal.h"


#define MB_REGISTER_BEDUG (true)


template <typename T>
class ModbusRegister
{
private:
    static constexpr const char* TAG = "MBR";

    register_type_t register_type;
    uint32_t id;
    uint32_t bytesSize;

    T deserializedData;
    std::shared_ptr<uint16_t[]> serializedData;


public:
    ModbusRegister(register_type_t register_type, uint32_t id, uint32_t bytesSize):
         register_type(register_type), id(id), bytesSize(bytesSize) { }

    ModbusRegister<T>(ModbusRegister<T>* other)
    {
        this->register_type = other->getType();
        this->id = other->getId();
        this->bytesSize = other->getBytesSize();
        this->set(other->get());
        this->serialize();
    }

    static std::shared_ptr<ModbusRegister<T>> createRegister(register_type_t register_type, uint32_t id, T value)
    {
        std::shared_ptr<ModbusRegister<T>> ptrMR = std::make_shared<ModbusRegister<T>>(register_type, id, static_cast<uint32_t>(sizeof(T)));
        ptrMR->set(value);
        ptrMR->serialize();
        return ptrMR;
    }

    T deserialize()
    {
        if (!serializedData) {
            return 0;
        }
        T value = 0;
#if MB_REGISTER_BEDUG
        LOG_BEDUG("%09lu->%s:\tdeserialize: ", HAL_GetTick(), TAG);
#endif
        for (unsigned i = 0; i < ModbusRegister::getRegSize(); i++) {
            uint16_t regValue = this->serializedData[i];
            T tmpValue = (((regValue >> 8) & 0x00FF) | ((regValue << 8) & 0xFF00));
            value |= (tmpValue << (16 * i));
#if MB_REGISTER_BEDUG
            LOG_BEDUG("%04X(value=%d) ", this->serializedData[i], static_cast<int>(this->serializedData[i]));
#endif
        }
#if MB_REGISTER_BEDUG
        LOG_BEDUG("- %d\n", static_cast<int>(value));
#endif
        return value;
    }

    void serialize()
    {
        std::shared_ptr<uint16_t[]> dataPtr(new uint16_t[ModbusRegister<T>::getRegSize()], [] (uint16_t* arr) {
            delete [] arr;
        });
        this->serializedData = dataPtr;
        memset(this->serializedData.get(), 0, ModbusRegister<T>::getRegSize());
        T value = this->deserializedData;
#if MB_REGISTER_BEDUG
        LOG_BEDUG("%09lu->%s:\tserialize: ", HAL_GetTick(), TAG);
#endif
        for (unsigned i = 0; i < sizeof(T); i+=2) {
            this->serializedData[i/2] = (static_cast<uint16_t>(value & 0xFF) << 8);
            value >>= 8;
            this->serializedData[i/2] |= (static_cast<uint16_t>(value & 0xFF));
            value >>= 8;
#if MB_REGISTER_BEDUG
            LOG_BEDUG("%04X ", this->serializedData[i/2]);
#endif
        }
#if MB_REGISTER_BEDUG
        LOG_BEDUG("\n");
#endif
    }

    T get()
    {
        return this->deserializedData;
    }

    void set(T value)
    {
        this->deserializedData = value;
    }

    std::shared_ptr<ModbusRegister<T>> save()
    {
        this->serialize();
        ModbusTableService::setRegisters(register_type, id, serializedData, ModbusRegister<T>::getRegSize());
        this->set(this->deserialize());
#if MB_REGISTER_BEDUG
        LOG_TAG_BEDUG(ModbusRegister::TAG, "modbus set id=%lu reg_type=0x%02X regs_count=%lu: %d", id, register_type, ModbusRegister<T>::getRegSize(), static_cast<int>(this->get()));
#endif
        return std::make_shared<ModbusRegister<T>>(this);
    }

    std::shared_ptr<ModbusRegister<T>> load()
    {
        serializedData = ModbusTableService::getRegisters(register_type, id, ModbusRegister<T>::getRegSize());
        this->set(this->deserialize());
#if MB_REGISTER_BEDUG
        LOG_TAG_BEDUG(ModbusRegister::TAG, "modbus get id=%lu reg_type=0x%02X regs_count=%lu: %d", id, register_type, ModbusRegister<T>::getRegSize(), static_cast<int>(this->get()));
#endif
        return std::make_shared<ModbusRegister<T>>(this);
    }

    register_type_t getType()
    {
        return this->register_type;
    }

    uint32_t getId()
    {
        return this->id;
    }

    uint8_t  getBytesSize()
    {
        return this->bytesSize;
    }

    uint32_t getNextAddress()
    {
        return this->getId() + this->getRegSize();
    }

    static inline uint32_t getRegSize() { return ((sizeof(T) / 2) + (sizeof(T) % 2)); }
};


#endif
