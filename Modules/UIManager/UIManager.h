#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include "keyboard4x3_manager.h"


#define UI_BEDUG (true)


class UIManager
{
public:
	static void UIProccess();

private:
	static constexpr const char* TAG = "UI";

	static bool isStartPressed;
	static char constBuffer[KEYBOARD4X3_BUFFER_SIZE];

	static bool checkKeyboardStop();
	static bool checkKeyboardStart();

	static bool checkStop();
	static bool checkStart();

	static void clear();
};

class UIBaseFSM
{
public:
	UIBaseFSM();

	virtual void tick();
};

class UIInitFSM: public UIBaseFSM
{
public:
	UIInitFSM();

	void tick() override;
};


#endif
