#include "Consoles.hpp"
#include "Shell.hpp"
#include "version.h"

#if defined(NDSH_THREADING) && defined(NDSH_CURL)
	#include "CurlMulti.hpp"
#endif

#include <dswifi9.h>
#include <fat.h>
#include <nds.h>
#ifndef __BLOCKSDS__
	#include <wfc.h>
#endif

#include <thread>

static bool fsInit{}, wifiInit{};

bool Shell::wifiInitialized()
{
	return wifiInit;
}

bool Shell::fsInitialized()
{
	return fsInit;
}

void InitResources()
{
	// Consoles::GetStream(0) and std::cout both write to the first console,
	// so the choice between them doesn't matter
	auto &ostr = std::cout;

	if (isDSiMode())
		ostr << "DSi mode detected!\n";

	ostr << "initializing filesystem...";

	if (!fatInitDefault())
	{
		ostr << "\r\e[2K\e[91mfat init failed: filesystem commands will "
				"not work\n";
	}
	else
	{
		ostr << "\r\e[2K\e[92mfilesystem intialized!\n";
		fsInit = true;
	}

	ostr << "initializing wifi...";

#ifdef __BLOCKSDS__
	// for some reason INIT_ONLY just doesn't let us autoconnect in the future?
	// we will have to rely on WFC_CONNECT from now on 🤷‍♂️
	if (!Wifi_InitDefault(WFC_CONNECT | WIFI_ATTEMPT_DSI_MODE))
#else
	if (!wlmgrInitDefault() || !wfcInit())
#endif
		ostr << "\r\e[2K\e[91mwifi init failed: networking commands will not "
				"work\e[39m\n";
	else
	{
		ostr << "\r\e[2K\e[92mwifi initialized!\n\e[39mautoconnecting...";
		wifiInit = true;
		void subcommand_autoconnect(std::ostream & ostr); // from wifi.cpp
		subcommand_autoconnect(ostr);
	}

#if defined(NDSH_THREADING) && defined(NDSH_CURL)
	CurlMulti::Init();
#endif

	ostr << '\n';
}

void PrintGreeting(int console, bool clearScreen = true)
{
	auto &ostr =
#ifdef NDSH_MULTICONSOLE
		Consoles::GetStream(console)
#else
		std::cout
#endif
		;

	if (clearScreen)
		ostr << "\e[2J\e[H"; // Clear screen and move cursor to home
	ostr << "\e[92mnds-shell " << GIT_HASH << " (con" << console << ")\e[m\n\nPress START to start a shell\n\n";
}

#ifdef NDSH_MULTICONSOLE
bool running[Consoles::NUM_CONSOLES]{};

void ShellThread(int console)
{
	running[console] = true;
	Shell{console}.StartPrompt();
	PrintGreeting(console);
	running[console] = false;
}
#endif

int main2()
{
	defaultExceptionHandler();
#ifndef __BLOCKSDS__
	tickInit();
#endif
	Consoles::Init();
	InitResources();

	{ // because why not?
		void PrintVersions();
		PrintVersions();
	}

#if defined(NDSH_THREADING) && !defined(__BLOCKSDS__)
	// Threads created with the C/C++ APIs are minimum priority by default:
	// https://github.com/devkitPro/calico/blob/6d437b7651ba5c95036a90f534c30079bb926945/source/system/newlib_syscalls.c#L166
	// The main thread is higher initially. Make sure we can yield to the others.
	threadGetSelf()->prio = THREAD_MIN_PRIO;
#endif

	// Print initial greeting on all consoles
	PrintGreeting(0, false); // Don't clear con0, InitResources output is there
#ifdef NDSH_MULTICONSOLE
	for (int i = 1; i < Consoles::NUM_CONSOLES; ++i)
		PrintGreeting(i);
#endif

#if defined(NDSH_THREADING) && defined(NDSH_MULTICONSOLE)
	std::thread shellThreads[Consoles::NUM_CONSOLES];
#endif

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

		if ((down & KEY_START)
#ifdef NDSH_MULTICONSOLE
			&& !running[console]
#endif
		)
		{
			// std::thread has no way of knowing whether the thread is stopped,
			// so we have to tell it ourselves
#if defined(NDSH_THREADING) && defined(NDSH_MULTICONSOLE)
			auto &thread = shellThreads[console];
			if (thread.joinable())
				thread.join();
			shellThreads[console] = std::thread{ShellThread, console};
#else
			Shell{console}.StartPrompt();
			PrintGreeting(console);
#endif
		}
#ifdef NDSH_MULTICONSOLE
		else if (down & KEY_L)
			Consoles::switchConsoleLeft();
		else if (down & KEY_R)
			Consoles::switchConsoleRight();
		else if (down & KEY_SELECT)
			Consoles::toggleFocusedDisplay();
#endif
	}

#if defined(NDSH_THREADING) && defined(NDSH_MULTICONSOLE)
	// Let all the shells exit naturally, to save line history, etc
	for (auto &thread : shellThreads)
		if (thread.joinable())
			thread.join();
#endif
}
