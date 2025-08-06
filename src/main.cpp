#include "Consoles.hpp"
#include "ShellClass.hpp"

#include <dswifi9.h>
#include <fat.h>
#include <nds.h>
#include <wfc.h>

#include <thread>

void subcommand_autoconnect(std::ostream &ostr); // from wifi.cpp

void InitResources()
{
	auto &ostr = Consoles::streams[0];

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

void ConsoleThread(int console)
{
	bool printed{}, firstPrint{true};
	auto &ostr = Consoles::streams[console];

	while (pmMainLoop())
	{
		threadYield();

		if (Consoles::focused_console != console && !firstPrint)
			// we want the console prompt shown on all consoles initially
			continue;

		if (!printed)
		{
			// we need to show the resource initialization on con0
			// only clear screen after it's been used
			if (!firstPrint)
				ostr << "\e[2J\e[H";

			ostr << "\e[42mnds-shell (con" << console << ")\e[39m\n\nPress START to start a shell";
			printed = true;
			firstPrint = false;
		}

		// main thread already called scanKeys(), let's trust it
		const auto keys = keysDown();

		if (keysDown() & KEY_START)
		{
			ostr << "\r\e[2K";
			Shell{console}.StartPrompt();
			printed = false;
		}
	}
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

	// Start shell threads
	std::thread threads[Consoles::NUM_CONSOLES];
	for (int i = 0; i < Consoles::NUM_CONSOLES; ++i)
		threads[i] = std::thread{ConsoleThread, i};

	// Main thread: Handle console switching and key scanning.
	while (pmMainLoop())
	{
		threadYield();
		scanKeys();

		const auto keys = keysDown();

		if (keys & KEY_SELECT)
			Consoles::toggleFocusedDisplay();

		if (keys & KEY_L)
			Consoles::handleKeyL();

		if (keys & KEY_R)
			Consoles::handleKeyR();
	}

	// TODO: save line history?
}
