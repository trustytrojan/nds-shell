#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Commands.hpp"

#include <dswifi9.h>
#include <fat.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;

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
		std::cerr << "\e[41mfatInitDefault failed: filesystem commands will "
					 "not work\e[39m\n\n";

	defaultExceptionHandler();

	// Initialize wifi
	if (!Wifi_InitDefault(false))
		std::cerr << "\e[41mWifi_InitDefault failed: networking commands will "
					 "not work\e[39m\n\n";
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

void SourceFile(const std::string &filepath)
{
	std::ifstream file{filepath};

	if (!file)
	{
		std::cerr << "\e[41mshell: cannot open file: " << filepath
				  << "\e[39m\n";
		return;
	}

	std::string line;
	while (std::getline(file, line) && !line.starts_with("return"))
		Shell::ProcessLine(line);
}

void ProcessLine(const std::string &line)
{
	// Split the line by whitespace into a vector of strings
	std::istringstream iss{line};

	args.clear();
	std::string token;
	while (iss >> token)
		args.emplace_back(token);

	if (args.empty())
	{
		std::cerr << "\e[41mshell: empty args\e[39m\n";
		return;
	}

	const auto &command = args[0];

	if (const auto withExtension{command + ".ndsh"}; fs::exists(withExtension))
	{
		SourceFile(withExtension);
		return;
	}

	const auto commandItr = Commands::MAP.find(command);

	if (commandItr == Commands::MAP.cend())
	{
		std::cerr << "\e[41mshell: unknown command\e[39m\n";
		return;
	}

	commandItr->second();
}

void Start()
{
	std::cout << "\e[46mgithub.com/trustytrojan/nds-shell\e[39m\n\n";

	if (fs::exists(".ndshrc"))
		SourceFile(".ndshrc");

	std::cout << "enter 'help' to see available\ncommands\n\n";

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
