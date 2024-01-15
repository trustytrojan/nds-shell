#include <nds.h>
#include <dswifi9.h>
#include <list>
#include <string>
#include <iostream>

void initializeConsole(void)
{
	static PrintConsole console;

	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);

	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
	// consoleInit(&screen.bottom, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);

	consoleSelect(&console);
}

int main(void)
{
	initializeConsole();

	const auto keyboard = keyboardDemoInit();
	keyboardShow();

	keyboard->OnKeyPressed = [](const auto key)
	{ if (key > 0) std::cout << (char)key; };

	std::string command;

	while (true)
	{
		std::cout << "> ";
		std::cin >> command;

		if (command == "hello")
		{
			std::cout << "world\n";
		}

		else if (command == "exit")
		{
			break;
		}

		else
		{
			std::cout << "unknown command\n";
		}
	}
}
