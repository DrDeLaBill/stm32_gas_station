/* Copyright © 2023 Georgy E. All rights reserved. */

#include "Pump.h"

#include <memory>
#include <string.h>
#include <typeinfo>

#include "UI.h"

#include "main.h"
#include "utils.h"


#define PUMP_MD212_MLS_PER_TICK         ((int32_t)25)
#define PUMP_MD212_TRIGGER_VAL_MAX      ((int32_t)30000)
#define PUMP_MD212_MLS_COUNT_MIN        ((uint32_t)10)
#define PUMP_MD212_MEASURE_DELAY_MS     ((uint32_t)300)

#define PUMP_ADC_READ_TIMEOUT_MS        ((uint32_t)100)
#define PUMP_ADC_MEASURE_DELAY_MS       ((uint32_t)5)
#define PUMP_ADC_OVERHEAD               ((uint32_t)4000)
#define PUMP_ADC_VALVE_MIN              ((uint32_t)1000)
#define PUMP_ADC_VALVE_MAX              ((uint32_t)2000)
#define PUMP_ADC_PUMP_MIN               ((uint32_t)1000)
#define PUMP_ADC_PUMP_MAX               ((uint32_t)3000)

#define PUMP_SESSION_ML_MAX             ((uint32_t)50000)
#define PUMP_SESSION_ML_MIN             ((uint32_t)1000)
#define PUMP_CHECK_START_DELAY_MS       ((uint32_t)5000)
#define PUMP_CHECK_STOP_DELAY_MS        ((uint32_t)5000)
#define PUMP_MEASURE_BUFFER_SIZE        ((uint8_t)10)

#define PUMP_SLOW_ML_VALUE              ((uint32_t)1000)


std::shared_ptr<PumpFSMBase> Pump::statePtr;
uint32_t Pump::lastUsedMl = 0;

uint32_t PumpFSMBase::targetMl = 0;
uint32_t PumpFSMBase::currentMlBase = 0;
int32_t PumpFSMBase::currentMlAdd = 0;
bool PumpFSMBase::hasError = false;
bool PumpFSMBase::hasStopped = false;
uint32_t PumpFSMBase::measureCounter = 0;
uint32_t PumpFSMBase::pumpBuf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
uint32_t PumpFSMBase::valveBuf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
int32_t PumpFSMBase::md212Buf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
util_timer_t PumpFSMBase::waitTimer = { 0 };
util_timer_t PumpFSMBase::errorTimer = { 0 };
bool PumpFSMBase::needStart = false;
bool PumpFSMBase::needStop = false;

#if PUMP_BEDUG
uint32_t PumpFSMBase::debugTicksBase = 0;
uint32_t PumpFSMBase::debugTicksAdd = 0;
#endif



void Pump::tick()
{
    if (!statePtr) {
        Pump::reset();
        statePtr = std::make_shared<PumpFSMInit>();
    }
    statePtr->proccess();
}

void Pump::measure()
{
    if (!statePtr) {
        Pump::reset();
    }
    statePtr->measure();
}

void Pump::reset()
{
    Pump::statePtr = std::make_shared<PumpFSMStop>();

    resetEncoder();
}

void Pump::resetEncoder()
{
    __HAL_TIM_SET_COUNTER(&MD212_TIM, Pump::getPumpEncoderMiddle());
}

void Pump::start()
{
    Pump::statePtr->start();
}

void Pump::stop()
{
    Pump::statePtr->stop();
}

bool Pump::isGunOnBase()
{
    return HAL_GPIO_ReadPin(GUN_SWITCH_GPIO_Port, GUN_SWITCH_Pin);
}

void Pump::clear()
{
    statePtr->clear();
}

void Pump::setTargetMl(uint32_t targetMl)
{
    statePtr->setTargetMl(targetMl);
}

uint32_t Pump::getCurrentMl()
{
    return statePtr->getCurrentMl();
}

constexpr uint32_t Pump::getPumpEncoderMiddle()
{
    return ((uint32_t)0xFFFF / 2);
}

void Pump::setLastMl(uint32_t lastMl)
{
    if (lastMl > 0) {
        UI::needToLoad();
    }
    Pump::lastUsedMl = lastMl;
}

uint32_t Pump::getLastMl()
{
    return Pump::lastUsedMl;
}

bool Pump::hasError()
{
    return statePtr->foundError();
}

bool Pump::hasStopped()
{
    return statePtr->pumpHasStopped();
}

PumpFSMBase::PumpFSMBase()
{
    PumpFSMBase::reset();
}

void PumpFSMBase::measure()
{
    if (util_is_timer_wait(&waitTimer)) {
        return;
    }

    if (measureCounter >= __arr_len(pumpBuf)) {
        return;
    }

    util_timer_start(&waitTimer, PUMP_ADC_MEASURE_DELAY_MS);
    pumpBuf[measureCounter]  = this->getADCPump();
    valveBuf[measureCounter] = this->getADCValve();
    md212Buf[measureCounter] = this->getEncoderTicks();
    measureCounter++;
}

void PumpFSMBase::start()
{
    needStart = true;
}

void PumpFSMBase::stop()
{
    needStop = true;
}

void PumpFSMBase::setTargetMl(uint32_t targetMl)
{
    if (hasError) {
        return;
    }
//    if (targetMl > PUMP_SESSION_ML_MAX) {
//        targetMl = PUMP_SESSION_ML_MAX;
//    }
    if (targetMl < PUMP_SESSION_ML_MIN) {
        return;
    }
    PumpFSMBase::targetMl = targetMl;
}

uint32_t PumpFSMBase::getCurrentMl()
{
    return currentMlBase + currentMlAdd;
}

int32_t PumpFSMBase::getCurrentEncoderMl()
{
    return (getEncoderTicks() * PUMP_MD212_MLS_PER_TICK) / 10;
}

uint32_t PumpFSMBase::getDebugTicks()
{
    return debugTicksBase + debugTicksAdd;
}

bool PumpFSMBase::isEnabled()
{
#if PUMP_PROTECT_ENABLE
    uint32_t pump_average  = this->getAverage(pumpBuf, __arr_len(pumpBuf));
//    uint32_t valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));

//    pump_average = PUMP_ADC_PUMP_MIN; // TODO: only for tests
    if (pump_average < PUMP_ADC_PUMP_MIN) {
        return false;
    }
//    if (valve_average < PUMP_ADC_VALVE_MIN) {
//        return false;
//    }

    return true;
#else
    return (typeid(*(Pump::statePtr)) == typeid(PumpFSMCheckStart)) ||
           (typeid(*(Pump::statePtr)) == typeid(PumpFSMWork));
#endif
}

bool PumpFSMBase::isOperable()
{
#if PUMP_PROTECT_ENABLE
    uint32_t pump_average  = this->getAverage(pumpBuf, __arr_len(pumpBuf));
//    uint32_t valve_average = _pump_get_average(pump_state.valve_measure_buf, __arr_len(pump_state.valve_measure_buf));

    if (pump_average > PUMP_ADC_PUMP_MAX) {
        return false;
    }
//    if (valve_average > PUMP_ADC_VALVE_MAX) {
//        return false;
//    }

    return true;
#else
    return true;
#endif
}

uint32_t PumpFSMBase::getAverage(uint32_t* data, uint32_t len)
{
    if (len == 0) {
        return 0;
    }

    uint32_t average = 0;

    for (uint8_t i = 0; i < len; i++) {
        average += data[i];
    }

    return average / len;
}

uint32_t PumpFSMBase::getAverageDelta(uint32_t* data, uint32_t len)
{
    if (len <= 1) {
        return 0;
    }

    int32_t average_difference = 0;

    for (uint8_t i = 0; i < len - 1; i++) {
        average_difference += __abs_dif(data[i], data[i + 1]);
    }

    return average_difference / len;
}

void PumpFSMBase::setPumpPower(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(PUMP_GPIO_Port, PUMP_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(PUMP_GPIO_Port, PUMP_Pin, enable_state);
}

void PumpFSMBase::setValve1Power(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(VALVE1_GPIO_Port, VALVE1_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(VALVE1_GPIO_Port, VALVE1_Pin, enable_state);
}

void PumpFSMBase::setValve2Power(GPIO_PinState enable_state)
{
    GPIO_PinState state = HAL_GPIO_ReadPin(VALVE2_GPIO_Port, VALVE2_Pin);
    if (state == enable_state) {
        return;
    }
    HAL_GPIO_WritePin(VALVE2_GPIO_Port, VALVE2_Pin, enable_state);
}

void PumpFSMBase::reset()
{
//    targetMl = 0;
    hasError = false;
    measureCounter = false;
    memset(pumpBuf, 0, sizeof(pumpBuf));
    memset(valveBuf, 0, sizeof(valveBuf));
    memset(md212Buf, 0, sizeof(md212Buf));
    memset(reinterpret_cast<void*>(&waitTimer), 0, sizeof(waitTimer));
    memset(reinterpret_cast<void*>(&errorTimer), 0, sizeof(errorTimer));
    needStart = false;
    needStop = false;
}

void PumpFSMBase::clear()
{
    targetMl      = 0;
    currentMlBase = 0;
    currentMlAdd  = 0;
    needStart     = false;
    needStop      = false;
    hasStopped    = false;
#if PUMP_BEDUG
    debugTicksBase = 0;
    debugTicksAdd  = 0;
#endif
}

uint32_t PumpFSMBase::getADCPump()
{
    ADC_ChannelConfTypeDef conf = {
        .Channel      = PUMP_ADC_CHANNEL,
        .Rank         = 1,
        .SamplingTime = ADC_SAMPLETIME_28CYCLES,
    };
    if (HAL_ADC_ConfigChannel(&PUMP_ADC, &conf) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&PUMP_ADC);
    HAL_ADC_PollForConversion(&PUMP_ADC, PUMP_ADC_READ_TIMEOUT_MS);
    uint32_t value = HAL_ADC_GetValue(&PUMP_ADC);
    HAL_ADC_Stop(&PUMP_ADC);

    return value;
}

uint32_t PumpFSMBase::getADCValve()
{
    ADC_ChannelConfTypeDef conf = {
        .Channel      = VALVE_ADC_CHANNEL,
        .Rank         = 1,
        .SamplingTime = ADC_SAMPLETIME_28CYCLES,
    };
    if (HAL_ADC_ConfigChannel(&VALVE_ADC, &conf) != HAL_OK) {
        return 0;
    }

    HAL_ADC_Start(&VALVE_ADC);
    HAL_ADC_PollForConversion(&VALVE_ADC, PUMP_ADC_READ_TIMEOUT_MS);
    uint32_t value = HAL_ADC_GetValue(&VALVE_ADC);
    HAL_ADC_Stop(&VALVE_ADC);

    return value;
}

int32_t PumpFSMBase::getEncoderTicks()
{
    uint32_t value = __HAL_TIM_GET_COUNTER(&MD212_TIM);
    int32_t result = 0;
    if (value >= Pump::getPumpEncoderMiddle()) {
        result = (int32_t)(value - Pump::getPumpEncoderMiddle());
    } else {
        result = -((int32_t)(Pump::getPumpEncoderMiddle() - value));
    }
    if (__abs(result) <= PUMP_MD212_MLS_COUNT_MIN) {
        return 0;
    }
    return result;
}

bool PumpFSMBase::pumpHasStopped()
{
    return hasStopped;
}

bool PumpFSMBase::foundError()
{
    return hasError;
}

void PumpFSMBase::setError()
{
#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMBase->PumpFSMError");
#endif
    Pump::statePtr = std::make_shared<PumpFSMError>();
}

void PumpFSMInit::proccess()
{
    (std::make_shared<PumpFSMStop>())->proccess();
#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMInit->PumpFSMWaitLIters");
#endif
    Pump::statePtr = std::make_shared<PumpFSMWaitLiters>();
}

void PumpFSMWaitLiters::proccess()
{
    if (targetMl > 0) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWaitLiters->PumpFSMWaitStart");
#endif
        Pump::statePtr = std::make_shared<PumpFSMWaitStart>();
        Pump::statePtr->start();
    }
}

void PumpFSMWaitStart::proccess()
{
    if (needStop) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWaitStart->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
        return;
    }

    if (needStart && targetMl > 0) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWaitStart->PumpFSMStart");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStart>();
        return;
    }

    if (util_is_timer_wait(&waitTimer)) {
        return;
    }

    if (measureCounter < __arr_len(pumpBuf)) {
        return;
    }

    measureCounter = 0;

    if (this->isEnabled() || !this->isOperable()) {
        this->setError();
        return;
    }
}

void PumpFSMStart::proccess()
{
    hasStopped = false;

    if (needStop) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMStart->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
        return;
    }

    if (Pump::isGunOnBase()) {
        return;
    }
    Pump::resetEncoder();

    this->setValve1Power(GPIO_PIN_SET);
    this->setPumpPower(GPIO_PIN_SET);
    this->setValve2Power(GPIO_PIN_SET);

#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMStart->PumpFSMCheckStart");
#endif
    Pump::statePtr = std::make_shared<PumpFSMCheckStart>();

    util_timer_start(&errorTimer, PUMP_CHECK_START_DELAY_MS);
}

void PumpFSMCheckStart::proccess()
{
    if (!util_is_timer_wait(&errorTimer)) {
        this->setError();
        return;
    }

    if (measureCounter < __arr_len(pumpBuf)) {
        return;
    }

    if (!this->isEnabled() || !this->isOperable()) {
        this->setError();
        return;
    }

#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMCheckStart->PumpFSMWork");
#endif
    Pump::statePtr = std::make_shared<PumpFSMWork>();
}

void PumpFSMWork::proccess()
{
    if (needStop) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWork->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
        return;
    }

    if (measureCounter < __arr_len(pumpBuf)) {
        return;
    }

    measureCounter = 0;

    if (!this->isEnabled() || !this->isOperable()) {
        this->setError();
        return;
    }

    int32_t currentEncoderMl = getCurrentEncoderMl();
    if (currentEncoderMl < 0) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "pump isn't working: current gas ticks=%ld; target=%lu", currentEncoderMl, targetMl);
#endif
        return;
    }

    if (Pump::isGunOnBase()) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWork->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
    }

    if (__abs_dif(static_cast<uint32_t>(currentEncoderMl), Pump::getCurrentMl()) == 0) {
        return;
    }

    debugTicksAdd = this->getEncoderTicks();
    if (this->getEncoderTicks() >= PUMP_MD212_TRIGGER_VAL_MAX) {
        Pump::resetEncoder();
        currentMlBase += currentMlAdd;
        currentMlAdd = 0;
#if PUMP_BEDUG
        debugTicksBase += debugTicksAdd;
        debugTicksAdd = 0;
#endif
    }

    if (currentMlAdd == getCurrentEncoderMl()) {
        return;
    }

#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "current gas ml: %lu (%ld ticks, current tim: %lu ticks); target: %lu", Pump::getCurrentMl(), PumpFSMBase::getDebugTicks(), debugTicksAdd, targetMl);
#endif

    currentMlAdd = getCurrentEncoderMl();


    uint32_t fastMlTarget =
        targetMl > PUMP_SLOW_ML_VALUE ?
        targetMl - PUMP_SLOW_ML_VALUE :
        0;
    if (Pump::getCurrentMl() >= fastMlTarget) { // TODO: if ml_current_count < 0 -> error
        this->setValve1Power(GPIO_PIN_RESET);
    }

    if (Pump::getCurrentMl() >= targetMl) {
#if PUMP_BEDUG
        LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMWork->PumpFSMStop (end)");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
    }
}

void PumpFSMStop::proccess()
{
    this->setPumpPower(GPIO_PIN_RESET);
    this->setValve1Power(GPIO_PIN_RESET);
    this->setValve2Power(GPIO_PIN_RESET);

#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMStop->PumpFSMCheckStop");
#endif
    Pump::statePtr = std::make_shared<PumpFSMCheckStop>();

    util_timer_start(&errorTimer, PUMP_CHECK_STOP_DELAY_MS);
}

void PumpFSMCheckStop::proccess()
{
    if (!util_is_timer_wait(&errorTimer)) {
        this->setError();
        return;
    }

    if (measureCounter < __arr_len(pumpBuf)) {
        return;
    }

    if (this->isEnabled() || !this->isOperable()) {
        this->setError();
        return;
    }

#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMCheckStop->PumpFSMrecord");
#endif
    Pump::statePtr = std::make_shared<PumpFSMRecord>();

    hasStopped = true;

    currentMlAdd = getCurrentEncoderMl();
}

void PumpFSMRecord::proccess()
{
    if (Pump::getCurrentMl() > 0) {
        Pump::setLastMl(Pump::getCurrentMl());
    }
#if PUMP_BEDUG
    LOG_TAG_BEDUG(Pump::TAG, "set PumpFSMRecord->PumpFSMWaitLiters");
    LOG_TAG_BEDUG(Pump::TAG, "Result TIM ticks: %lu", PumpFSMBase::getDebugTicks());
#endif
    Pump::statePtr = std::make_shared<PumpFSMWaitLiters>();

    targetMl = 0;

    Pump::resetEncoder();
}

void PumpFSMError::proccess()
{
    hasError = true;
    Pump::setLastMl(0);

    this->setPumpPower(GPIO_PIN_RESET);
    this->setValve1Power(GPIO_PIN_RESET);
    this->setValve2Power(GPIO_PIN_RESET);
}
