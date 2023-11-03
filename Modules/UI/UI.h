#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include <memory>

#include "utils.h"
#include "keyboard4x3_manager.h"


#define UI_BEDUG (true)


class UIFSMBase
{
public:
    UIFSMBase(uint32_t delay);

    void tick();

    bool hasError();

protected:
    util_timer_t timer;
    bool hasErrors;

    static uint32_t targetMl;

    virtual bool checkState();
    virtual void proccess() {}
};

class UIFSMInit: public UIFSMBase
{
public:
    UIFSMInit();

protected:
    void proccess() override;
};

class UIFSMLoad: public UIFSMBase
{
public:
    UIFSMLoad();

protected:
    void proccess() override;
};

class UIFSMWait: public UIFSMBase
{
public:
    UIFSMWait();

protected:
    void proccess() override;
};

class UIFSMInput: public UIFSMBase
{
public:
    UIFSMInput();

protected:
    void proccess() override;

private:
    static const uint32_t INPUT_DELAY = 30000;

};

class UIFSMStart: public UIFSMBase
{
public:
    UIFSMStart();

protected:
    void proccess() override;
};

class UIFSMWaitCount: public UIFSMBase
{
public:
    UIFSMWaitCount(uint32_t lastMl);

protected:
    uint32_t lastMl;

    void proccess() override;
    bool checkState() override;

private:
    static const uint32_t COUNT_DELAY = 120000;

};

class UIFSMCount: public UIFSMWaitCount
{
public:
    UIFSMCount(uint32_t lastMl);

protected:
    void proccess() override;
    bool checkState() override;

};

class UIFSMStop: public UIFSMBase
{
public:
    UIFSMStop();

protected:
    void proccess() override;

};

class UIFSMResult: public UIFSMBase
{
public:
    UIFSMResult();

protected:
    void proccess() override;
    bool checkState() override;

private:
    static const uint32_t RESULT_DELAY = 300000;

};

class UIFSMError: public UIFSMBase
{
public:
    UIFSMError();
};


class UI
{
public:
    static constexpr const char TAG[] = "UI ";

    static std::shared_ptr<UIFSMBase> ui;
    static uint8_t result[KEYBOARD4X3_BUFFER_SIZE];

    static void UIProccess();

    static bool checkStop();
    static bool checkStart();
    static bool checkGunOnBase();

    static void setLoad();
    static void resetLoad();
    static bool needToLoad();

    static void setCard(uint32_t card);
    static uint32_t getCard();

private:
    static bool needLoad;
    static uint32_t lastCard;

    static bool checkErrors();

    static bool checkKeyboardStop();
    static bool checkKeyboardStart();
};


#endif
