#pragma once

#include "CliPrompt.hpp"
#include "Consoles.hpp"

#include <nds.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Env = std::unordered_map<std::string, std::string>;

class Shell
{
	std::ostream &ostr;

	// global environment
	Env env;

	// for redirections
	std::ofstream outf, errf;
	std::ifstream inf;

	// to pass to commands
	std::ostream *out{&ostr}, *err{&ostr};
	std::istream *in{&std::cin}; // please do NOT use normal stdin for ANYTHING 😭
	std::vector<std::string> args;

	CliPrompt prompt;

	bool shouldExit{};

	void RedirectOutput(int fd, const std::string &filename);
	void RedirectInput(int fd, const std::string &filename);
	void ResetStreams();

public:
	const int console;

	Shell(int console);
	void StartPrompt();
	void ProcessLine(std::string_view line);
	void SourceFile(const std::string &path);
	void SetShouldExit() { shouldExit = true; }
	bool IsFocused() const { return Consoles::IsFocused(console); }

	// For POSIX shell "builtin" commands like cd, history, unset, etc.
	void SetEnv(const std::string &key, const std::string &value) { env.insert_or_assign(key, value); }
	void UnsetEnv(const std::string &key) { env.erase(key); }
	const std::vector<std::string> &GetLineHistory() const { return prompt.getLineHistory(); }
	void ClearLineHistory() { prompt.clearLineHistory(); }
};
