#ifndef UI_MANAGER_H
#define UI_MANAGER_H


#include "keyboard4x3_manager.h"


class UIManager
{
public:
	static void UIProccess();

private:
	static bool isStartPressed;
	static char constBuffer[KEYBOARD4X3_BUFFER_SIZE];

	static bool checkKeyboardStop();
	static bool checkKeyboardStart();
};


#endif
