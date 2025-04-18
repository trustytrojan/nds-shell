#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "EscapeSequences.hpp"

#include <dswifi9.h>
#include <fat.h>

#include <iostream>
#include <sstream>

namespace Shell
{

void Init()
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	// Show console on top screen
	static PrintConsole console;
	consoleInit(
		&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

	// Show keyboard on bottom screen
	keyboardDemoInit();
	keyboardShow();

	// Mount sdcard using libfat
	if (!fatInitDefault())
		std::cerr << "fatInitDefault failed: filesystem commands will "
					 "not work\n\n"_brightred;

	defaultExceptionHandler();

	// Initialize wifi
	if (!Wifi_InitDefault(false))
		std::cerr << "Wifi_InitDefault failed: networking commands will "
					 "not work\n\n"_brightred;
}

std::string EscapeEscapes(const std::string &str)
{
	std::string newStr;
	for (auto itr = str.cbegin(); itr < str.cend(); ++itr)
		switch (*itr)
		{
		case '\\':
			if (*(++itr) == 'e')
				newStr += '\e';
			else
			{
				newStr += '\\';
				--itr;
			}
			break;
		default:
			newStr += *itr;
		}
	return newStr;
}

void ProcessLine(std::string &line)
{
	// Split the line by whitespace into a vector of strings
	std::istringstream iss{line};

	args.clear();
	std::string token;
	while (iss >> token)
		args.push_back(token);

	if (args.empty())
	{
		std::cerr << "shell: empty args\n"_brightred;
		return;
	}

	const auto commandItr = Commands::MAP.find(args[0]);

	if (commandItr == Commands::MAP.cend())
	{
		std::cerr << "shell: unknown command\n"_brightred;
		return;
	}

	commandItr->second();
}

void Start()
{
	std::cout << "github.com/trustytrojan/nds-shell\n\n"_brightcyan
			  << "enter 'help' to see available\ncommands\n\n";

	CliPrompt prompt;
	std::string line;

	while (pmMainLoop())
	{
		swiWaitForVBlank();
		prompt.getLine(line);

		// Trim leading and trailing whitespace
		line.erase(0, line.find_first_not_of(" \t\n\r"));
		line.erase(line.find_last_not_of(" \t\n\r") + 1);

		if (line.empty())
			continue;

		ProcessLine(line);
	}
}

} // namespace Shell
