#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "Consoles.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

std::ostream &operator<<(std::ostream &ostr, const Token &t)
{
	return ostr << (char)t.type << '(' << t.value << ')';
}

Shell::Shell(const int console)
	: ostr{Consoles::GetStream(console)},
	  console{console}
{
	prompt.setOutputStream(ostr);
	if (!fsInitialized())
		return;
	prompt.setLineHistory(".ndsh_history");
	if (fs::exists(".ndshrc"))
		SourceFile(".ndshrc");
}

Shell::~Shell()
{
	// allow disabling history saving, melonDS freezes upon writing any file to disk
	if (!fsInitialized() || env.contains("DONT_SAVE_HISTORY"))
		return;

	// save everything afterwards; opening files in append mode corrupts them.
	// may just be a limitation of dkp's libfat
	std::ofstream historyFile{".ndsh_history"};
	for (const auto &line : prompt.getLineHistory())
		historyFile << line << '\n';
}

void Shell::SourceFile(const std::string &filepath)
{
	if (!fsInitialized())
	{
		ostr << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	std::error_code ec;
	if (!fs::exists(filepath, ec))
	{
		ostr << "\r[41mshell: file does not exist: " << filepath << "\e[39m\n";
		return;
	}

	std::ifstream file{filepath};
	if (!file)
	{
		ostr << "\e[91mshell: cannot open file: " << filepath << "\e[39m\n";
		return;
	}

	std::string line;
	while (std::getline(file, line) && !line.starts_with("return"))
		ProcessLine(line);
}

void Shell::ProcessLine(std::string_view line)
{
	if (line.empty())
		return;

	// Trim leading and trailing whitespace
	const auto first_non_whitespace = line.find_first_not_of(" \t\n\r");
	line = line.substr(first_non_whitespace, line.find_last_not_of(" \t\n\r") - first_non_whitespace + 1);

	if (line.empty())
		return;

	std::vector<Token> tokens;

	if (!LexLine(tokens, line, env))
		return;

	if (env.contains("DEBUG"))
	{ // debug tokens
		ostr << "\e[90mtokens: ";
		auto itr = tokens.cbegin();
		for (; itr < tokens.cend() - 1; ++itr)
			ostr << *itr << ' ';
		ostr << *itr << "\n\e[39m";
	}

	std::vector<std::string> args;
	std::vector<IoRedirect> redirects;
	std::vector<EnvAssign> envAssigns;

	if (!ParseTokens(tokens, redirects, envAssigns, args))
		return;

	if (args.empty() && envAssigns.empty())
	{
		ostr << "\e[91mshell: no args or env assigns\e[39m\n";
		return;
	}

	// Handle environment assignments.

	if (args.empty())
	{ // Standalone assignments - apply to shell env and return
		env.insert_range(envAssigns);
		return;
	}

	if (env.contains("DEBUG"))
	{ // debug args
		ostr << "\e[90margs: ";
		auto itr = args.cbegin();
		for (; itr < args.cend() - 1; ++itr)
			ostr << '\'' << *itr << "' ";
		ostr << '\'' << *itr << "'\n\e[39m";
	}

	// Command with env assignments. Copy shell env then assign to it.
	Env commandEnv{env};
	commandEnv.insert_range(envAssigns);

	// Apply redirections (rightmost takes precedence for same fd)
	std::vector<std::istream *> opened_istreams;
	std::vector<std::ostream *> opened_ostreams;
	for (const auto &redirect : redirects)
	{
		if (redirect.direction == IoRedirect::Direction::IN)
		{
			if (const auto p = RedirectInputFromFile(redirect.fd, redirect.filename))
				opened_istreams.emplace_back(p);
		}
		else if (redirect.direction == IoRedirect::Direction::OUT)
		{
			if (const auto p = RedirectOutputToFile(redirect.fd, redirect.filename))
				opened_ostreams.emplace_back(p);
		}
	}

	const auto &command = args[0];

	if (const auto withExtension{command + ".ndsh"}; fsInitialized() && fs::exists(withExtension))
	{ // Treat .ndsh files as commands!
		SourceFile(withExtension);
		return;
	}

	if (const auto itr = Commands::MAP.find(command); itr != Commands::MAP.cend())
		itr->second({*out, *err, *in, args, commandEnv, *this});
	else
		ostr << "\e[91mshell: unknown command\e[39m\n";

	// free/delete the opened streams (which closes them too)!
	for (const auto p : opened_istreams)
		if (p) // null check for sanity
			delete p;
	for (const auto p : opened_ostreams)
		if (p)
			delete p;

	ResetStreams();
}

void Shell::ResetStreams()
{
	if (in != &std::cin)
		in = &std::cin;
	if (out != &ostr)
		out = &ostr;
	if (err != &ostr)
		err = &ostr;
}

std::ostream *Shell::RedirectOutputToFile(int fd, const std::string &filename)
{
	if (!fsInitialized())
	{
		ostr << "\e[91mshell: fs not initialized\e[39m\n";
		return {};
	}

	const auto fstrp = new std::ofstream{filename};
	if (!fstrp)
	{
		ostr << "\e[91mshell: cannot allocate ofstream\e[39m\n";
		return {};
	}

	auto &fstr = *fstrp;
	if (!fstr)
	{
		ostr << "\e[91mshell: cannot open file for writing: " << filename << "\e[39m\n";
		return {};
	}

	RedirectOutput(fd, fstr);
	return fstrp;
}

std::istream *Shell::RedirectInputFromFile(int fd, const std::string &filename)
{
	if (!fsInitialized())
	{
		ostr << "\e[91mshell: fs not initialized\e[39m\n";
		return {};
	}

	const auto fstrp = new std::ifstream{filename};
	if (!fstrp)
	{
		ostr << "\e[91mshell: cannot allocate ifstream\e[39m\n";
		return {};
	}

	auto &fstr = *fstrp;
	if (!fstr)
	{
		ostr << "\e[91mshell: cannot open file for reading: " << filename << "\e[39m\n";
		return {};
	}

	RedirectInput(fd, fstr);
	return fstrp;
}

void Shell::RedirectInput(int fd, std::istream &istr)
{
	if (fd == 0)
		in = &istr;
	// in the future, a full file descriptor table???????
}

void Shell::RedirectOutput(int fd, std::ostream &ostr)
{
	if (fd == 1)
		out = &ostr;
	else if (fd == 2)
		err = &ostr;
	// in the future, a full file descriptor table???????
}

void Shell::StartPrompt()
{
	ostr << "\e[96mtrustytrojan/nds-shell\nstar me on github!!!\e[39m\n\nrun 'help' for help\n\n";

	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	while (pmMainLoop() && !shouldExit)
	{
#ifdef NDSH_THREADING
		threadYield();

		if (!Consoles::IsFocused(console))
			continue;
#else
		swiWaitForVBlank();
#endif

		prompt.update();

		if (prompt.enterPressed())
		{
			ProcessLine(prompt.getInput());
			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}

		if (prompt.foldPressed())
			// fold key exits, just like the other commands
			break;
	}
}
