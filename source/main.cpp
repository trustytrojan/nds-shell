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
		std::cerr << "fatInitDefault failed: filesystem commands will not work\n";
}

int main(void)
{
	InitializeResources();
	std::cout << "check out the repo!\ngithub.com/trustytrojan/nds-shell\n";
	StartShell("> ", '_', ProcessLine);
}
