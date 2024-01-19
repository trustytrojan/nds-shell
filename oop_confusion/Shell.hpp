#ifndef __SHELL__
#define __SHELL__

#include "Prompt.hpp"

/**
 * Represents a `bash`-line shell for the Nintendo DS.
 */
struct Shell : Prompt
{
	std::vector<std::string> commandHistory;
	std::unordered_map<std::string, std::string> environmentVariables;
};

#endif