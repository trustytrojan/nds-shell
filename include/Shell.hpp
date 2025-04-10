#pragma once

#include "Lexer.hpp"
#include "libdeps.hpp"

namespace Shell
{
// Initializes necessary libnds resources.
void Init();

// Starts the prompt loop. Does not return.
void Start();

bool RedirectInput(const int fd, const std::string &filename);
bool RedirectOutput(const int fd, const std::string &filename);

// A command's arguments
inline std::vector<std::string> args;

// Shell environment variables
inline EnvMap env{{"PS1", "> "}, {"CURSOR", "_"}};

namespace Commands
{
void help();
void echo();
void envCmd();
void cd();
void ls();
void cat();
void rm();
void dns();
void wifi();
void http();

const std::unordered_map<std::string, void (*)()> map{
	{"help", help},
	{"exit", systemShutDown},
	{"clear",
	 []()
	 {
		 std::cout << "\e[2J";
	 }},
	{"ls", ls},
	{"cat", cat},
	{"echo", echo},
	{"env", envCmd},
	{"dns", dns},
	// {"wifi", wifi},
	{"http", http}};
} // namespace Commands
}; // namespace Shell
