#pragma once

#include <nds.h>

#include <string>
#include <vector>

namespace Shell
{

// Initializes necessary libnds resources.
void Init();

// Starts the prompt loop. Does not return.
void Start();

inline std::string cwd;

// A command's arguments
inline std::vector<std::string> args;

}; // namespace Shell
