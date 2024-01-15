#include <nds.h>
#include <dswifi9.h>
#include <list>
#include <string>
#include <iostream>

void consoleInit(void)
{
	static PrintConsole topScreen;
	// PrintConsole bottomScreen;

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&topScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	// consoleInit(&bottomScreen, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	consoleSelect(&topScreen);
}

int main(void) {
	consoleInit();
	const auto keyboard = keyboardDemoInit();
	keyboardShow();

	std::list<std::string> commandHistory;
	std::string command;

	keyboard->OnKeyPressed = [](int key)
	{
		if (key > 0) std::cout << (char)key;
	};

	while (true)
	{
		std::cout << "> ";
		std::cin >> command;

		if (command == "hello")
		{
			std::cout << "world\n";
		}

		else
		{
			std::cout << "unknown command\n";
		}

		commandHistory.push_back(command);
	}
}
