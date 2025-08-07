#include "Shell.hpp"
#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "Consoles.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"

#include <filesystem>

namespace fs = std::filesystem;

std::ostream &operator<<(std::ostream &ostr, const Token &t)
{
	return ostr << (char)t.type << '(' << t.value << ')';
}

Shell::Shell(const int console)
	: ostr{Consoles::GetStream(console)},
	  console{console}
{
	if (fs::exists(".ndshrc"))
		SourceFile(".ndshrc");
	prompt.setOutputStream(ostr);
	prompt.setLineHistory(".ndsh_history");
}

Shell::~Shell()
{
	// save everything afterwards; opening files in append mode corrupts them.
	// may just be a limitation of dkp's libfat
	std::ofstream historyFile{".ndsh_history"};
	for (const auto &line : prompt.getLineHistory())
		historyFile << line << '\n';
}

void Shell::SourceFile(const std::string &filepath)
{
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

	if (env.contains("SHELL_DEBUG"))
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

	if (env.contains("SHELL_DEBUG"))
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
	ResetStreams(); // always reset first
	for (const auto &redirect : redirects)
		if (redirect.direction == IoRedirect::Direction::IN)
			RedirectInput(redirect.fd, redirect.filename);
		else
			RedirectOutput(redirect.fd, redirect.filename);

	const auto &command = args[0];

	// ostr << "\e[90mcommand: '" << command << "'\n";

	if (const auto withExtension{command + ".ndsh"}; fs::exists(withExtension))
	{ // Treat .ndsh files as commands!
		SourceFile(withExtension);
		return;
	}

	if (const auto itr = Commands::MAP.find(command); itr != Commands::MAP.cend())
		itr->second({*out, *err, *in, args, commandEnv, *this});
	else
		ostr << "\e[91mshell: unknown command\e[39m\n";
}

void Shell::ResetStreams()
{
	if (in == &inf)
	{
		inf.close();
		in = &std::cin;
	}

	if (out == &outf)
	{
		outf.close();
		out = &ostr;
	}

	if (err == &errf)
	{
		errf.close();
		err = &ostr;
	}
}

void Shell::RedirectOutput(int fd, const std::string &filename)
{
	if (fd == 1)
	{
		outf.open(filename);
		if (!outf)
		{
			ostr << "\e[91mshell: cannot open file for writing: " << filename << "\e[39m\n";
			return;
		}
		out = &outf;
	}
	else if (fd == 2)
	{
		errf.open(filename);
		if (!errf)
		{
			ostr << "\e[91mshell: cannot open file for writing: " << filename << "\e[39m\n";
			return;
		}
		err = &errf;
	}
}

void Shell::RedirectInput(int fd, const std::string &filename)
{
	inf.open(filename);
	if (!inf)
	{
		ostr << "\e[91mshell: cannot open file for reading: " << filename << "\e[39m\n";
		return;
	}
	if (fd == 0)
		in = &inf;
}

void Shell::StartPrompt()
{
	ostr << "\e[96mgithub.com/trustytrojan/nds-shell\e[39m\n\nrun 'help' for help\n\n";

	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	while (pmMainLoop() && !shouldExit)
	{
		threadYield();

		if (!Consoles::IsFocused(console))
			continue;

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
