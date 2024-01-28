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
	// Setup prompt and line buffer
	CliPrompt prompt(env["PS1"], env["CURSOR"][0], std::cout);
	std::string line;

	// Print startup text
	std::cout << "\e[46mnds-shell\ngithub.com/trustytrojan\e[39m\n\nenter 'help' to see available\ncommands\n\n";

	std::vector<Token> tokens;

	// Start loop
	while (1)
	{
		// The important 3 statements:
		prompt.GetLine(line);
		if (!LexLine(tokens, line, env))
			std::cerr << "\e[41mshell: LexLine returned false\e[39m\n";
		line.clear();
		for (const auto &[type, value] : tokens)
		{
			std::cout << (char)type << '(' << value << ")\n";
		}
		tokens.clear();
		//ParseLine(line); // The rest of the program lies here

		// Update the prompt using env vars
		// Allow escapes in basePrompt, for colors
		prompt.basePrompt = EscapeEscapes(env["PS1"]); // If the user wants an empty prompt, so be it
		prompt.cursor = (env["CURSOR"].empty()) ? ' ' : env["CURSOR"][0]; // Avoid a segfault here...
	}
}

void NDS_Shell::ParseLine(const std::string &line)
{
	// save the hassle
	if (line.empty())
		return;

	const auto lineEnd = line.cend();
	std::string currentArg;
	std::vector<std::string> tempEnvKeys;

	for (auto itr = line.cbegin(); itr < lineEnd; ++itr)
	{
		if (isspace(*itr))
		{
			if (!currentArg.empty())
			{
				// End of current whitespace-separated string, push as an argument.
				args.push_back(currentArg);
				currentArg.clear();
			}

			// Nothing else needs to happen with whitespace
			continue;
		}

		// if (isdigit(*itr) && currentArg.empty())
		// {
		// 	if (!ParsePossibleRedirect(itr, lineEnd, currentArg))
		// 		return;
		// 	continue;
		// }

		switch (*itr)
		{
		case '\\':
			// In an unquoted string, only escape spaces, backslashes, and equals.
			// Otherwise, omit the backslash.
			switch (*(++itr))
			{
			case ' ':
				currentArg += ' ';
				break;
			case '=':
				currentArg += '=';
				break;
			case '\\':
				currentArg += '\\';
				break;
			default:
				currentArg += *itr;
			}
			break;

		case '"':
			if (ParseDoubleQuotedString(itr, lineEnd, currentArg))
			{
				args.push_back(currentArg);
				currentArg.clear();
			}
			else
				return;
			break;

		case '<':
			if (!ParseInputRedirect({0}, itr))
				return;
			break;

		case '>':
			if (!ParseOutputRedirect({1}, itr))
				return;
			break;

		default:
			currentArg += *itr;
		}
	}

	if (!currentArg.empty())
		args.push_back(currentArg);

	// Run command if necessary
	if (!args.empty())
	{
		const auto itr = Commands::map.find(args.front());
		if (itr == Commands::map.cend())
			std::cerr << "\e[41munknown command\e[39m\n";
		else
			itr->second();
	}

	// Clear command state
	args.clear();
	stdio.reset();
	for (const auto &key : tempEnvKeys)
		env.erase(key);
}

bool NDS_Shell::ParseDoubleQuotedString(std::string::const_iterator &itr, const std::string::const_iterator &lineEnd, std::string &currentArg)
{
	// When called, itr is pointing at the opening `"`, so increment before looping
	for (++itr; *itr != '"' && itr < lineEnd; ++itr)
		switch (*itr)
		{
		case '\\':
			// Only escape double-quotes.
			// Commands can further escape their arguments if needed.
			if (*(++itr) == '"')
				currentArg += '"';
			else
			{
				currentArg += '\\';
				--itr; // Cancel out the ++ used in the comparison
			}
			break;

		case '$':
		{
			std::string varname;
			for (++itr; (isalnum(*itr) || *itr == '_') && *itr != '"' && itr < lineEnd; ++itr)
				varname += *itr;
			currentArg += env[varname];
			if (*itr == '"')
				--itr; // Decrement to compensate for the outer loop's increment
			break;
		}

		default:
			currentArg += *itr;
		}

	if (itr == lineEnd)
	{
		std::cerr << "\e[41mshell: closing `\"` not found\e[39m\n";
		return false;
	}

	return true;
}

bool NDS_Shell::ParsePossibleRedirect(const std::string::const_iterator &startItr, const std::string::const_iterator &lineEnd, std::string &currentArg)
{
	std::vector<int> fds;
	auto itr = startItr;

	// Add fds to vector until we hit a non-digit or end of line
	for (; isdigit(*itr) && itr < lineEnd; ++itr)
		fds.push_back(*itr - 48);

	if (itr == lineEnd)
	{
		// This is not a redirect, just a string full of digits. Add it as an argument.
		for (itr = startItr; !isspace(*itr) && itr < lineEnd; ++itr)
			currentArg += *itr;
		args.push_back(currentArg);
		// No need to clear currentArg since we have hit end of line.
		// This is not an error, so return true.
		return true;
	}

	switch (*itr)
	{
	case '<':
		return ParseInputRedirect(fds, itr);
	case '>':
		return ParseOutputRedirect(fds, itr);
	default:
		// This is not a redirect, just a string. Add it as an argument.
		for (itr = startItr; !isspace(*itr) && itr < lineEnd; ++itr)
			currentArg += *itr;
		args.push_back(currentArg);
		currentArg.clear();

		// This is not an error, so return true.
		return true;
	}
}

bool NDS_Shell::ParseInputRedirect(const std::vector<int> fds, std::string::const_iterator &itr)
{
}

bool NDS_Shell::ParseOutputRedirect(const std::vector<int> fds, std::string::const_iterator &itr)
{
}
