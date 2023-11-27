/* Copyright © 2023 Georgy E. All rights reserved. */

#pragma once


#include <memory>
#include <stdint.h>

#include "utils.h"


#define PUMP_BEDUG                      (true)
#define PUMP_PROTECT_ENABLE             (false)

#define PUMP_MEASURE_BUFFER_SIZE        ((uint8_t)10)


class PumpFSMBase
{
public:
    PumpFSMBase();

    void measure();
    void start();
    void stop();
    void setTargetMl(uint32_t targetMl);

    static uint32_t getCurrentMl();
#if PUMP_BEDUG
    static uint32_t getDebugTicks();
#endif

    bool pumpHasStarted();
    bool pumpHasStopped();
    bool foundError();

    static void reset();
    static void clear();

    virtual void proccess() {}

protected:
    static uint32_t     targetMl;
    static bool         hasError;

    static uint32_t     measureCounter;
    static uint32_t     valveBuf[PUMP_MEASURE_BUFFER_SIZE];
    static uint32_t     pumpBuf [PUMP_MEASURE_BUFFER_SIZE];
    static int32_t      md212Buf[PUMP_MEASURE_BUFFER_SIZE];

    static uint32_t     currentMlBase;
    static int32_t      currentMlAdd;

    static util_timer_t waitTimer;
    static util_timer_t errorTimer;

    static bool         needStart;
    static bool         needStop;

    static bool         hasStarted;
    static bool         hasStopped;

#if PUMP_BEDUG
    static uint32_t     debugTicksBase;
    static uint32_t     debugTicksAdd;
#endif


    bool isEnabled();
    bool isOperable();

    uint32_t getAverage(uint32_t* data, uint32_t len);
    uint32_t getAverageDelta(uint32_t* data, uint32_t len);

    void setPumpPower(GPIO_PinState enable_state);
    void setValve1Power(GPIO_PinState enable_state);
    void setValve2Power(GPIO_PinState enable_state);

    void setError();

    static int32_t getCurrentEncoderMl();
    static int32_t getEncoderTicks();

private:
    uint32_t getADCPump();
    uint32_t getADCValve();

};

class PumpFSMInit: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMWaitLiters: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMWaitStart: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMStart: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMCheckStart: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMWork: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMStop: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMCheckStop: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMRecord: public PumpFSMBase
{
public:
    void proccess() override;
};

class PumpFSMError: public PumpFSMBase
{
public:
    void proccess() override;
};


class Pump
{
public:
    static constexpr const char* TAG = "PMP";

    static std::shared_ptr<PumpFSMBase> statePtr;

    static void tick();
    static void measure();

    static void reset();

    static void resetEncoder();

    static void start();
    static void stop();
    static void clear();
    static void setTargetMl(uint32_t targetMl);
    static uint32_t getCurrentMl();

    static constexpr uint32_t getPumpEncoderMiddle();

    static void setLastMl(uint32_t lastMl);
    static uint32_t getLastMl();

    static bool hasError();
    static bool hasStarted();
    static bool hasStopped();
    static bool isGunOnBase();

private:
    static uint32_t lastUsedMl;

};
