/* Copyright © 2023 Georgy E. All rights reserved. */

#include "ModbusRegister.h"

#include <memory>
#include <string.h>
#include <stdint.h>

#include "modbus_rtu_base.h"

#include "ModbusTableService.h"


//template<typename T>
//const char ModbusRegister<T>::TAG[] = "MBR";

//
//template<typename T>
//ModbusRegister<T>::ModbusRegister(register_type_t register_type, uint32_t id, uint32_t bytesSize):
// 	register_type(register_type), id(id), bytesSize(bytesSize) { }

//template<typename T>
//T ModbusRegister<T>::deserialize()
//{
//    if (!serializedData) {
//        return 0;
//    }
//	T value = 0;
//	for (unsigned i = 0; i < ModbusRegister::getRegSize(); i++) {
//	    T tmpValue = ((this->serializedData[i] >> 8) & 0xFF);
//		if (i != (sizeof(T) / 2)) {
//		    tmpValue |= (((this->serializedData[i] << 8) & 0xFF00));
//		}
//		value |= (tmpValue << (16 * i));
//	}
//	return value;
//}

//template<typename T>
//void ModbusRegister<T>::serialize()
//{
//	std::shared_ptr<uint16_t[]> dataPtr(new uint16_t[ModbusRegister<T>::getRegSize()], [] (uint16_t* arr) {
//        delete [] arr;
//    });
//    this->serializedData = dataPtr;
//	memset(this->serializedData.get(), 0, ModbusRegister<T>::getRegSize());
//	T value = this->deserializedData;
//	for (unsigned i = 0; i < sizeof(T); i+=2) {
//		this->serializedData[i/2] = (static_cast<uint16_t>(value & 0xFF) << 8);
//		value >>= 8;
//		this->serializedData[i/2] |= (static_cast<uint16_t>(value & 0xFF));
//		value >>= 8;
//	}
//}

template <typename T>
ModbusRegister<T> ModbusRegister<T>::operator=(const ModbusRegister modbusRegister)
{
	this->id = modbusRegister.id;
	this->bytesSize = modbusRegister.bytesSize;
	return *this;
}

//template <typename T>
//std::shared_ptr<ModbusRegister<T>> ModbusRegister<T>::createRegister(register_type_t register_type, uint32_t id, T value)
//{
//    std::shared_ptr<ModbusRegister<T>> ptrMR = std::make_shared<ModbusRegister<T>>(register_type, id, static_cast<uint32_t>(sizeof(T)));
//    ptrMR->set(value);
//    ptrMR->serialize();
//    return ptrMR;
//}

//template <typename T>
//void ModbusRegister<T>::save()
//{
//	LOG_TAG_BEDUG(ModbusRegister::TAG, "modbus set id=%lu reg_type=0x%02X regs_count=%lu: %d", id, register_type, ModbusRegister<T>::getRegSize(), this->deserializedData);
//	this->serialize();
//	ModbusTableService::setRegisters(register_type, id, serializedData, ModbusRegister<T>::getRegSize());
//}

//template <typename T>
//void ModbusRegister<T>::load()
//{
//	serializedData = ModbusTableService::getRegisters(register_type, id, ModbusRegister<T>::getRegSize());
//	LOG_TAG_BEDUG(ModbusRegister::TAG, "modbus get id=%lu reg_type=0x%02X regs_count=%lu: %d", id, register_type, ModbusRegister<T>::getRegSize(), this->deserialize());
//	this->deserialize();
//}

//template <typename T>
//T ModbusRegister<T>::get()
//{
//	return this->deserializedData;
//}

//template <typename T>
//void ModbusRegister<T>::set(T value)
//{
//	this->deserializedData = value;
//}

//template <typename T>
//uint32_t ModbusRegister<T>::getId()
//{
//	return this->id;
//}
//
//template <typename T>
//uint8_t ModbusRegister<T>::getBytesSize()
//{
//	return this->bytesSize;
//}

//template <typename T>
//uint32_t ModbusRegister<T>::getNextAddress()
//{
//	return this->getId() + this->getRegSize();
//}
