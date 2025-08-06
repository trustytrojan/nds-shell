#include "Consoles.hpp"
#include "Shell.hpp"

#include <algorithm>
#include <dswifi9.h>
#include <fat.h>
#include <nds.h>
#include <wfc.h>

#include <thread>

void subcommand_autoconnect(std::ostream &ostr); // from wifi.cpp

void InitResources()
{
	auto &ostr = Consoles::GetStream(0);

	ostr << "initializing filesystem...";

	if (!fatInitDefault())
		ostr << "\r\e[2K\e[41mfat init failed: filesystem commands will "
				"not work\e[39m\n";
	else
		ostr << "\r\e[2K\e[42mfilesystem intialized!\n";

	ostr << "initializing wifi...";

	if (!wlmgrInitDefault() || !wfcInit())
		ostr << "\r\e[2K\e[41mwifi init failed: networking commands will not "
				"work\e[39m\n";
	else
	{
		ostr << "\r\e[2K\e[42mwifi initialized!\n\e[39mautoconnecting...";
		subcommand_autoconnect(ostr);
	}

	ostr << '\n';
}

void PrintGreeting(int console, bool clearScreen = true)
{
	auto &ostr = Consoles::GetStream(console);
	if (clearScreen)
		ostr << "\e[2J\e[H"; // Clear screen and move cursor to home
	ostr << "\e[42mnds-shell (con" << console << ")\e[39m\n\nPress START to start a shell\n\n";
}

void ShellThread(int console)
{
	Shell{console}.StartPrompt();
	PrintGreeting(console);
}

int main()
{
	tickInit();
	defaultExceptionHandler();
	Consoles::Init();
	InitResources();

	// Threads created with the C/C++ APIs are minimum priority by default.
	// The main thread is higher. Make sure we can yield to the others.
	threadGetSelf()->prio = THREAD_MIN_PRIO;

	// Print initial greeting on all consoles
	PrintGreeting(0, false); // Don't clear con0, InitResources output is there
	for (int i = 1; i < Consoles::NUM_CONSOLES; ++i)
		PrintGreeting(i);

	// Need to keep them alive here, detach() doesn't work
	std::thread shellThreads[Consoles::NUM_CONSOLES];

	// Main thread: Handle console switching and key scanning.
	while (pmMainLoop())
	{
		threadYield();

		// This only needs to be called here and not in other threads,
		// since they all are run sequentially.
		scanKeys();

		const auto down = keysDown();
		const auto console = Consoles::GetFocusedConsole();

		if ((down & KEY_START) && !shellThreads[console].native_handle())
			shellThreads[console] = std::thread{ShellThread, console};
		else if (down & KEY_L)
			Consoles::switchConsoleLeft();
		else if (down & KEY_R)
			Consoles::switchConsoleRight();
		else if (down & KEY_SELECT)
			Consoles::toggleFocusedDisplay();
	}

	// Let all the shells exit naturally, to save line history, etc
	std::ranges::for_each(shellThreads, [](auto &t) { t.join(); });
}
