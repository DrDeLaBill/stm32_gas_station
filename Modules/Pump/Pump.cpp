/* Copyright © 2023 Georgy E. All rights reserved. */

#include "Pump.h"

#include <memory>
#include <string.h>
#include <typeinfo>

#include "UI.h"

#include "log.h"
#include "main.h"
#include "soul.h"
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
#define PUMP_CHECK_DELAY_MS             ((uint32_t)5000)
#define PUMP_MEASURE_BUFFER_SIZE        ((uint8_t)10)

#define PUMP_SLOW_ML_VALUE              ((uint32_t)1000)


std::shared_ptr<PumpFSMBase> Pump::statePtr;
uint32_t Pump::lastUsedMl = 0;

uint32_t PumpFSMBase::targetMl = 0;
uint32_t PumpFSMBase::currentMlBase = 0;
int32_t PumpFSMBase::currentMlAdd = 0;
int32_t PumpFSMBase::currentTicksBase = 0;
int32_t PumpFSMBase::currentTicksAdd = 0;
bool PumpFSMBase::hasStarted = false;
bool PumpFSMBase::hasStopped = false;
uint32_t PumpFSMBase::measureCounter = 0;
uint32_t PumpFSMBase::pumpBuf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
uint32_t PumpFSMBase::valveBuf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
int32_t PumpFSMBase::md212Buf[PUMP_MEASURE_BUFFER_SIZE] = { 0 };
utl::Timer PumpFSMBase::waitTimer(PUMP_ADC_MEASURE_DELAY_MS);
utl::Timer PumpFSMBase::errorTimer(PUMP_CHECK_DELAY_MS);
bool PumpFSMBase::needStart = false;
bool PumpFSMBase::needStop = false;



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
        UI::setLoading();
    }
    Pump::lastUsedMl = lastMl;
}

uint32_t Pump::getLastMl()
{
    return Pump::lastUsedMl;
}

bool Pump::hasError()
{
    return is_error(PUMP_FAULT);
}

bool Pump::hasStarted()
{
    return statePtr->pumpHasStarted();
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
    if (waitTimer.wait()) {
        return;
    }

    if (measureCounter >= __arr_len(pumpBuf)) {
        return;
    }

    waitTimer.start();
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
    if (is_error(PUMP_FAULT)) {
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

int32_t PumpFSMBase::getCurrentTicks()
{
	return currentTicksBase + currentTicksAdd;
}

int32_t PumpFSMBase::getCurrentEncoderMl()
{
    return (getEncoderTicks() * PUMP_MD212_MLS_PER_TICK) / 10;
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
    reset_error(PUMP_FAULT);
    measureCounter = false;
    memset(pumpBuf, 0, sizeof(pumpBuf));
    memset(valveBuf, 0, sizeof(valveBuf));
    memset(md212Buf, 0, sizeof(md212Buf));
    waitTimer.reset();
    errorTimer.reset();
    needStart = false;
    needStop = false;
}

void PumpFSMBase::clear()
{
    targetMl         = 0;
    currentMlBase    = 0;
    currentMlAdd     = 0;
    needStart        = false;
    needStop         = false;
    hasStarted       = false;
    hasStopped       = false;
    currentTicksBase = 0;
    currentTicksAdd  = 0;
}

uint32_t PumpFSMBase::getADCPump()
{
    ADC_ChannelConfTypeDef conf = {};
    conf.Channel      = PUMP_ADC_CHANNEL;
    conf.Rank         = 1;
    conf.SamplingTime = ADC_SAMPLETIME_28CYCLES;
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
    ADC_ChannelConfTypeDef conf = {};
    conf.Channel      = VALVE_ADC_CHANNEL;
    conf.Rank         = 1;
    conf.SamplingTime = ADC_SAMPLETIME_28CYCLES;
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
    int32_t value = __HAL_TIM_GET_COUNTER(&MD212_TIM);
    int32_t result = 0;
    if (value >= static_cast<int32_t>(Pump::getPumpEncoderMiddle())) {
        result = (int32_t)(value - Pump::getPumpEncoderMiddle());
    } else {
        result = -((int32_t)(Pump::getPumpEncoderMiddle() - value));
    }
    if (__abs(result) <= PUMP_MD212_MLS_COUNT_MIN) {
        return 0;
    }
    return result;
}

void PumpFSMBase::setEncoderTicks(int32_t ticks)
{
	__HAL_TIM_SET_COUNTER(&MD212_TIM, ticks + static_cast<int32_t>(Pump::getPumpEncoderMiddle()));
}

bool PumpFSMBase::pumpHasStarted()
{
    return hasStarted;
}

bool PumpFSMBase::pumpHasStopped()
{
    return hasStopped;
}

void PumpFSMBase::setError()
{
#if PUMP_BEDUG
    printTagLog(Pump::TAG, "set PumpFSMBase->PumpFSMError");
#endif
    Pump::statePtr = std::make_shared<PumpFSMError>();
}

void PumpFSMInit::proccess()
{
#if PUMP_BEDUG
    printTagLog(Pump::TAG, "set PumpFSMInit->PumpFSMStop");
#endif
    Pump::statePtr = std::make_shared<PumpFSMStop>();
}

void PumpFSMWaitLiters::proccess()
{
    if (targetMl > 0) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWaitLiters->PumpFSMWaitStart");
#endif
        Pump::statePtr = std::make_shared<PumpFSMWaitStart>();
        Pump::statePtr->start();
    }
}

void PumpFSMWaitStart::proccess()
{
    if (needStop) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWaitStart->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
        return;
    }

    if (needStart && targetMl > 0) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWaitStart->PumpFSMStart");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStart>();
        return;
    }

    if (waitTimer.wait()) {
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
    hasStarted = false;

    if (needStop) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMStart->PumpFSMStop");
#endif
        Pump::reset();
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
    printTagLog(Pump::TAG, "set PumpFSMStart->PumpFSMCheckStart");
#endif
    Pump::statePtr = std::make_shared<PumpFSMCheckStart>();

    errorTimer.start();
}

void PumpFSMCheckStart::proccess()
{
    if (!errorTimer.wait()) {
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

    hasStarted = true;

#if PUMP_BEDUG
    printTagLog(Pump::TAG, "set PumpFSMCheckStart->PumpFSMWork");
#endif
    Pump::statePtr = std::make_shared<PumpFSMWork>();
}

void PumpFSMWork::proccess()
{
    if (needStop) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWork->PumpFSMStop");
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

    if (Pump::isGunOnBase()) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWork->PumpFSMStop");
#endif
        Pump::statePtr = std::make_shared<PumpFSMStop>();
    }

    int32_t currentEncoderMl = getCurrentEncoderMl();
    if (this->getEncoderTicks() < currentTicksAdd) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "pump isn't working: current gas ticks=%ld; target=%lu", currentTicksAdd, targetMl);
#endif
        setEncoderTicks(currentTicksAdd);
    }

    if (__abs_dif(static_cast<uint32_t>(currentEncoderMl), Pump::getCurrentMl()) == 0) {
        return;
    }

    if (this->getEncoderTicks() >= PUMP_MD212_TRIGGER_VAL_MAX) {
        Pump::resetEncoder();
        currentMlBase += currentMlAdd;
        currentMlAdd = 0;
        currentTicksBase += currentTicksAdd;
        currentTicksAdd = 0;
    }

    if (currentMlAdd == getCurrentEncoderMl()) {
        return;
    }

#if PUMP_BEDUG
    printTagLog(Pump::TAG, "current gas ml: %lu (%ld ticks, current tim: %lu ticks); target: %lu", Pump::getCurrentMl(), PumpFSMBase::getCurrentTicks(), currentTicksAdd, targetMl);
#endif

    currentMlAdd = getCurrentEncoderMl();
    currentTicksAdd = getEncoderTicks();


    uint32_t fastMlTarget =
        targetMl > PUMP_SLOW_ML_VALUE ?
        targetMl - PUMP_SLOW_ML_VALUE :
        0;
    if (Pump::getCurrentMl() >= fastMlTarget) { // TODO: if ml_current_count < 0 -> error
        this->setValve1Power(GPIO_PIN_RESET);
    }

    if (Pump::getCurrentMl() >= targetMl) {
#if PUMP_BEDUG
        printTagLog(Pump::TAG, "set PumpFSMWork->PumpFSMStop (end)");
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
    printTagLog(Pump::TAG, "set PumpFSMStop->PumpFSMCheckStop");
#endif
    Pump::statePtr = std::make_shared<PumpFSMCheckStop>();

    hasStarted = false;

    errorTimer.start();
}

void PumpFSMCheckStop::proccess()
{
    if (!errorTimer.wait()) {
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
    printTagLog(Pump::TAG, "set PumpFSMCheckStop->PumpFSMRecord");
#endif
    Pump::statePtr = std::make_shared<PumpFSMRecord>();

    hasStopped = true;

    currentMlAdd = getCurrentEncoderMl();
    currentTicksAdd = getEncoderTicks();
    if (currentMlAdd < 0) {
    	currentMlAdd = 0;
    }
    if (currentTicksAdd < 0) {
    	currentTicksAdd = 0;
    }
}

void PumpFSMRecord::proccess()
{
//    if (Pump::getCurrentMl() > 0) {
//        Pump::setLastMl(Pump::getCurrentMl());
//    }
#if PUMP_BEDUG
    printTagLog(Pump::TAG, "set PumpFSMRecord->PumpFSMWaitLiters");
    printTagLog(Pump::TAG, "Result TIM ticks: %lu", PumpFSMBase::getCurrentTicks());
#endif
    Pump::statePtr = std::make_shared<PumpFSMWaitLiters>();

    targetMl = 0;

    Pump::resetEncoder();
}

void PumpFSMError::proccess()
{
    set_error(PUMP_FAULT);
//    Pump::setLastMl(0);

    this->setPumpPower(GPIO_PIN_RESET);
    this->setValve1Power(GPIO_PIN_RESET);
    this->setValve2Power(GPIO_PIN_RESET);
}
