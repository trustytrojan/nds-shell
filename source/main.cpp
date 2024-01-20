#include "everything.hpp"

Keyboard *keyboard;

void InitializeResources(void)
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	// Show the console on the top screen
	static PrintConsole console;
	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

	// Initialize and show the keyboard on the bottom screen
	keyboard = keyboardDemoInit();
	keyboardShow();

	// Mount the sdcard using libfat
	if (!fatInitDefault())
		std::cerr << "\e[31mfatInitDefault failed: filesystem commands will not work\e[39m\n\n";

	defaultExceptionHandler();

	if (!Wifi_InitDefault(false))
		std::cerr << "\e[31mWifi_InitDefault failed: networking commands will not work\e[39m\n\n";
}

int main(void)
{
	InitializeResources();
	std::cout << "\e[46mnds-shell\ngithub.com/trustytrojan\e[39m\n\nenter 'help' to see available\ncommands\n\n";
	StartShell("> ", '_', ProcessLine, std::cout);
}
