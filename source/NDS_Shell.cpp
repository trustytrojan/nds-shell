#include "NDS_Shell.hpp"
#include "CliPrompt.hpp"
#include "Lexer.hpp"

void NDS_Shell::Init()
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	// Show console on top screen
	static PrintConsole console;
	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

	// Show keyboard on bottom screen
	keyboardDemoInit();
	keyboardShow();

	// Mount sdcard using libfat
	if (!fatInitDefault())
		std::cerr << "\e[41mfatInitDefault failed: filesystem commands will not work\e[39m\n\n";

	defaultExceptionHandler();

	// Initialize wifi
	if (!Wifi_InitDefault(false))
		std::cerr << "\e[41mWifi_InitDefault failed: networking commands will not work\e[39m\n\n";
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

void NDS_Shell::Start()
{
	// Print startup text
	std::cout << "\e[46mnds-shell\ngithub.com/trustytrojan\e[39m\n\nenter 'help' to see available\ncommands\n\n";

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
		prompt.basePrompt = EscapeEscapes(env["PS1"]); // If the user wants an empty prompt, so be it
		prompt.cursor = env["CURSOR"].empty() ? ' ' : env["CURSOR"][0]; // Avoid a segfault here...

		line.clear();
		tokens.clear();
	}
}

bool NDS_Shell::ParseTokens(const std::vector<Token> &tokens)
{
	const auto tokensBegin = tokens.cbegin();
	const auto tokensEnd = tokens.cend();

	for (auto itr = tokensBegin; itr < tokensEnd; ++itr)
		// first, process all operators
		switch (itr->type)
		{
		case Token::Type::INPUT_REDIRECT:
			if (!ParseInputRedirect(itr, tokensBegin, tokensEnd))
				return false;
			break;
		
		// case Token::Type::OUTPUT_REDIRECT:
		// 	if (!ParseOutputRedirect(itr, tokensBegin, tokensEnd))
		// 		return false;
		// 	break;
		}

	return true;
}

bool NDS_Shell::ParseInputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
{
	const auto prevItr = itr - 1;
	const auto nextItr = itr + 1;

	if (nextItr == tokensEnd || nextItr->type != Token::Type::STRING)
	{
		std::cerr << "\e[41mshell: filename expected after `<`\e[39m\n";
		return false;
	}

	const auto &filename = nextItr->value;

	if (!std::filesystem::exists(filename))
	{
		std::cerr << "\e[41mshell: file `" << filename << "` does not exist\e[39m\n";
		return false;
	}

	// if `<` is the first token, or whitespace came before it
	if (itr == tokensBegin || prevItr->type == Token::Type::WHITESPACE)
	{
		// redirect the file to stdin
		RedirectInput(0, filename);
		return true;
	}

	if (prevItr->type == Token::Type::STRING)
	{
		const auto &fdStr = prevItr->value;

		if (!std::ranges::all_of(fdStr, isdigit))
		{
			std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
			return false;
		}

		RedirectInput(atoi(fdStr.c_str()), filename);
		return true;
	}

	std::cerr << "\e[41mshell: integer expected before <\e[39m\n";
	return false;
}

bool NDS_Shell::RedirectInput(const int fd, const std::string &filename)
{

}
