#pragma once

#include "CliPrompt.hpp"
#include "Consoles.hpp"

#include <nds.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Env = std::unordered_map<std::string, std::string>;

class Shell
{
public:
	static bool fsInitialized();
	static bool wifiInitialized();

private:
	// main output stream for the shell itself
	std::ostream &ostr;

	// Environment variable table
	Env env;

	// to pass to commands
	std::ostream *out{&ostr}, *err{&ostr};
	std::istream *in{&std::cin}; // please do NOT use normal stdin for ANYTHING 😭
	std::vector<std::string> args;

	// The shell's prompt
	CliPrompt prompt;

	// Whether the shell loop should break on the next iteration.
	// This is set by the `exit` command.
	bool shouldExit{};

	// Creates a `std::ofstream` from `filename` and calls `RedirectOutput(fd, fstr)`.
	// The returned `std::ostream *` MUST be free'd/deleted.
	std::ostream *RedirectOutputToFile(int fd, const std::string &filename);

	// Creates a `std::ifstream` from `filename` and calls `RedirectInput(fd, fstr)`.
	// The returned `std::istream *` MUST be free'd/deleted.
	std::istream *RedirectInputFromFile(int fd, const std::string &filename);

	// Sets `in`, `out`, and `err` back to their defaults.
	// Does NOT free any resources that they previously pointed to!
	void ResetStreams();

	// Autocomplete callback for CliPrompt.
	void AutocompleteCallback(const std::string &input, std::vector<std::string> &options);

public:
	const int console;

	Shell(int console);
	~Shell();
	void StartPrompt();
	void ProcessLine(std::string_view line);
	void SourceFile(const std::string &path);

	// Send command output (`ctx.out` or `ctx.err`) to `ostr`.
	void RedirectOutput(int fd, std::ostream &ostr);

	// Command input (`ctx.in`) is read from `istr`.
	void RedirectInput(int fd, std::istream &istr);

	// Set the `shouldExit` flag. If set the shell will exit the prompt loop on its next iteration.
	void SetShouldExit() { shouldExit = true; }

	// Returns whether the console this shell is running on is focused.
	// **Commands should use this to avoid calling `CliPrompt::update()`
	// as to not consome inputs that a focused console should receive.**
	bool IsFocused() const { return Consoles::IsFocused(console); }

	// For POSIX shell "builtin" commands like cd, history, unset, etc.
	void SetEnv(const std::string &key, const std::string &value) { env.insert_or_assign(key, value); }
	void UnsetEnv(const std::string &key) { env.erase(key); }
	const std::vector<std::string> &GetLineHistory() const { return prompt.getLineHistory(); }
	void ClearLineHistory() { prompt.clearLineHistory(); }
};
