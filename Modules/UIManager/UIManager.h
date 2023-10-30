#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include <memory>

#include "utils.h"
#include "keyboard4x3_manager.h"


#define UI_BEDUG (true)


class UIFSMBase
{
public:
	UIFSMBase();

	void tick();

protected:
	util_timer_t timer;
	bool hasErrors;

	virtual void checkState();
	virtual void proccess() {}

	virtual constexpr uint32_t getTimerDelayMs() { return 0; }
};

class UIFSMInit: public UIFSMBase
{
public:
	UIFSMInit() { UIFSMBase(); }

protected:
	void proccess() override;
};

class UIFSMLoad: public UIFSMBase
{
public:
	UIFSMLoad() { UIFSMBase(); }

protected:
	void proccess() override;
};

class UIFSMWait: public UIFSMBase
{
public:
	UIFSMWait() { UIFSMBase(); }

protected:
	void proccess() override;
};

class UIFSMInput: public UIFSMBase
{
public:
	UIFSMInput() { UIFSMBase(); }

protected:
	void proccess() override;

	constexpr uint32_t getTimerDelayMs() override { return 30000; }
};

class UIFSMStart: public UIFSMBase
{
public:
	UIFSMStart() { UIFSMBase(); }

protected:
	void proccess() override;
};

class UIFSMCount: public UIFSMBase
{
public:
	UIFSMCount() { UIFSMBase(); }

protected:
	void proccess() override;

	constexpr uint32_t getTimerDelayMs() override { return 300000; }
};

class UIFSMStop: public UIFSMBase
{
public:
	UIFSMStop() { UIFSMBase(); }

protected:
	void proccess() override;
};

class UIFSMResult: public UIFSMBase
{
public:
	UIFSMResult() { UIFSMBase(); }

protected:
	void checkState() override;

	constexpr uint32_t getTimerDelayMs() override { return 300000; }
};

class UIFSMError: public UIFSMBase
{
public:
	UIFSMError() { UIFSMBase(); this->hasErrors = true; }

protected:
	void proccess() override;
};


class UIManager
{
public:
	static constexpr const char TAG[] = "UI";

	static std::unique_ptr<UIFSMBase> ui;

	static void UIProccess();

	static bool checkKeyboardStop();
	static bool checkKeyboardStart();

	static void clear();

private:
	static bool checkErrors();

	static bool checkStop();
	static bool checkStart();
};


#endif
