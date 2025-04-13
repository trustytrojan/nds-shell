#pragma once

#include "Lexer.hpp"
#include <nds.h>

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

}; // namespace Shell
