#ifndef UI_MANAGER_H
#define UI_MANAGER_H


class UIManager
{
public:
	static void UIProccess();

private:
	static void gasProccess();
	static bool checkKeyboardStart();
	static bool checkKeyboardStop();
};


#endif
