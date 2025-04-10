#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

void Shell::Init()
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

void Shell::Start()
{
	// Print startup text
	std::cout << "\e[46mnds-shell\ngithub.com/trustytrojan\e[39m\n\nenter "
				 "'help' to see available\ncommands\n\n";

	// Setup prompt
	CliPrompt prompt(env["PS1"], env["CURSOR"][0], std::cout);

	// Line buffer and token vector
	std::string line;
	std::vector<Token> tokens;

	while (1)
	{
		prompt.GetLine(line);

		if (!LexLine(tokens, line, env))
			// error occurred, continue
			continue;

		// Debug loop to check tokens
		for (const auto &[type, value] : tokens)
			std::cout << (char)type << '(' << value << ")\n";

		if (!ParseTokens(tokens))
			// error occurred, continue
			continue;

		// Update the prompt using env vars
		// Allow escapes in basePrompt, for colors
		prompt.basePrompt = EscapeEscapes(
			env["PS1"]); // If the user wants an empty prompt, so be it
		prompt.cursor = env["CURSOR"].empty()
							? ' '
							: env["CURSOR"][0]; // Avoid a segfault here...

		line.clear();
		tokens.clear();
	}
}

bool Shell::RedirectInput(const int fd, const std::string &filename)
{
	std::cout << "Redirecting input from " << filename << " to " << fd << '\n';
}

bool Shell::RedirectOutput(const int fd, const std::string &filename)
{
	std::cout << "Redirecting output from " << fd << " to " << filename << '\n';
}
