#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <dswifi9.h>
#include <fat.h>

#include <filesystem>
#include <fstream>
#include <iostream>

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
	if (!Wifi_InitDefault(false)) // PASS TRUE FOR EMULATORS
		std::cerr << "\e[41mWifi_InitDefault failed: networking commands will "
					 "not work\e[39m\n\n";
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
	if (!line.length())
		return;

	std::vector<Token> tokens;

	if (!LexLine(tokens, line, env))
		return;

	args.clear();

	if (!ParseTokens(tokens))
		return;

	if (args.empty())
	{
		std::cerr << "\e[41mshell: empty args\e[39m\n";
		return;
	}

	std::cerr << "args: ";
	for (const auto &arg : args)
		std::cerr << "'" << arg << "' ";
	std::cerr << '\n';

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
	ResetStreams();
}

void ResetStreams()
{
	if (in != &std::cin)
	{
		reinterpret_cast<std::ifstream *>(in)->close();
		in = &std::cin;
	}

	if (out != &std::cout)
	{
		reinterpret_cast<std::ofstream *>(out)->close();
		out = &std::cout;
	}

	if (err != &std::cerr)
	{
		reinterpret_cast<std::ofstream *>(err)->close();
		err = &std::cerr;
	}
}

void RedirectOutput(int fd, const std::string &filename)
{
	if (fd == 1)
	{
		outf.open(filename);
		if (!outf)
		{
			*err << "\e[41mshell: cannot open file for writing: " << filename
				 << "\e[39m\n";
			return;
		}
		out = &outf;
	}
	else if (fd == 2)
	{
		errf.open(filename);
		if (!errf)
		{
			*err << "\e[41mshell: cannot open file for writing: " << filename
				 << "\e[39m\n";
			return;
		}
		err = &errf;
	}
}

void RedirectInput(int fd, const std::string &filename)
{
	inf.open(filename);
	if (!inf)
	{
		*err << "\e[41mshell: cannot open file for reading: " << filename
			 << "\e[39m\n";
		return;
	}
	if (fd == 0)
		in = &inf;
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
