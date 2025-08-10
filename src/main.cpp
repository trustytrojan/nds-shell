#include "Consoles.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <fat.h>
#include <nds.h>
#include <wfc.h>

#include <thread>

void subcommand_autoconnect(std::ostream &ostr); // from wifi.cpp

static bool fsInit{};

bool Shell::fsInitialized()
{
	return fsInit;
}

void InitResources()
{
	auto &ostr = Consoles::GetStream(0);

	ostr << "initializing filesystem...";

	if (!fatInitDefault())
		ostr << "\r\e[2K\e[41mfat init failed: filesystem commands will "
				"not work\e[39m\n";
	else
	{
		ostr << "\r\e[2K\e[42mfilesystem intialized!\n";
		fsInit = true;
	}

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

bool running[Consoles::NUM_CONSOLES]{};

void ShellThread(int console)
{
	running[console] = true;
	Shell{console}.StartPrompt();
	PrintGreeting(console);
	running[console] = false;
}

int main()
{
	defaultExceptionHandler();
	tickInit();
	Consoles::Init();
	InitResources();

#ifdef NDSH_THREADING
	// Threads created with the C/C++ APIs are minimum priority by default:
	// https://github.com/devkitPro/calico/blob/6d437b7651ba5c95036a90f534c30079bb926945/source/system/newlib_syscalls.c#L166
	// The main thread is higher initially. Make sure we can yield to the others.
	threadGetSelf()->prio = THREAD_MIN_PRIO;
#endif

	// Print initial greeting on all consoles
	PrintGreeting(0, false); // Don't clear con0, InitResources output is there
	for (int i = 1; i < Consoles::NUM_CONSOLES; ++i)
		PrintGreeting(i);

	std::thread shellThreads[Consoles::NUM_CONSOLES];

	// Main thread: Handle console switching and key scanning.
	while (pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		// This only needs to be called here and not in other threads,
		// since they all are run sequentially.
		scanKeys();

		const auto down = keysDown();
		const auto console = Consoles::GetFocusedConsole();

		if ((down & KEY_START) && !running[console])
		{
			// std::thread has no way of knowing whether the thread is stopped,
			// so we have to tell it ourselves
#ifdef NDSH_THREADING
			auto &thread = shellThreads[console];
			if (thread.joinable())
				thread.join();
			shellThreads[console] = std::thread{ShellThread, console};
#else
			ShellThread(console);
#endif
		}
		else if (down & KEY_L)
			Consoles::switchConsoleLeft();
		else if (down & KEY_R)
			Consoles::switchConsoleRight();
		else if (down & KEY_SELECT)
			Consoles::toggleFocusedDisplay();
	}

	// Let all the shells exit naturally, to save line history, etc
	for (auto &thread : shellThreads)
		if (thread.joinable())
			thread.join();
}
