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
		std::cerr << "\e[41mfatInitDefault failed: filesystem commands may not "
					 "work\e[39m\n";

	defaultExceptionHandler();

	// Initialize wifi
	// print something here to indicate wifi connection attempt!
	std::cerr << "\e[40mattempting wifi autoconnect...\n\e[39m";
	if (!Wifi_InitDefault(true))
		std::cerr << "\e[41mWifi_InitDefault failed: networking commands may "
					 "not work\e[39m\n";
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

std::ostream &operator<<(std::ostream &ostr, const Token &t)
{
	return ostr << (char)t.type << '(' << t.value << ')';
}

bool HasEnv(const std::string &key)
{
	return commandEnv.find(key) != commandEnv.cend() ||
		   env.find(key) != env.cend();
}

std::optional<std::string> GetEnv(const std::string &key)
{
	if (const auto itr = commandEnv.find(key); itr != commandEnv.cend())
		return itr->second;
	if (const auto itr = env.find(key); itr != env.cend())
		return itr->second;
	return {};
}

std::string GetEnv(const std::string &key, const std::string &_default)
{
	const auto val = GetEnv(key);
	return val ? *val : _default;
}

void ProcessLine(const std::string &line)
{
	if (!line.length())
		return;

	std::vector<Token> tokens;

	if (!LexLine(tokens, line, env))
		return;

	if (HasEnv("SHELL_DEBUG"))
	{ // debug tokens
		std::cerr << "\e[40mtokens: ";
		auto itr = tokens.cbegin();
		for (; itr < tokens.cend() - 1; ++itr)
			std::cerr << *itr << ' ';
		std::cerr << *itr << "\n\e[39m";
	}

	args.clear();
	commandEnv.clear(); // Clear previous command-local env
	std::vector<IoRedirect> redirects;
	std::vector<EnvAssign> envAssigns;

	if (!ParseTokens(tokens, redirects, envAssigns, args))
		return;

	if (args.empty() && envAssigns.empty())
	{
		std::cerr << "\e[41mshell: no args or env assigns\e[39m\n";
		return;
	}

	// Handle environment assignments
	if (args.empty())
	{
		// Standalone assignments - apply to shell env
		for (const auto &assign : envAssigns)
			env[assign.key] = assign.value;
		return;
	}

	// Command with assignments - apply to command env
	for (const auto &assign : envAssigns)
		commandEnv[assign.key] = assign.value;

	// Apply redirections (rightmost takes precedence for same fd)
	for (const auto &redirect : redirects)
		if (redirect.direction == IoRedirect::Direction::IN)
			RedirectInput(redirect.fd, redirect.filename);
		else
			RedirectOutput(redirect.fd, redirect.filename);

	if (HasEnv("SHELL_DEBUG"))
	{ // debug args
		std::cerr << "\e[40margs: ";
		auto itr = args.cbegin();
		for (; itr < args.cend() - 1; ++itr)
			std::cerr << '\'' << *itr << "' ";
		std::cerr << '\'' << *itr << "'\n\e[39m";
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
