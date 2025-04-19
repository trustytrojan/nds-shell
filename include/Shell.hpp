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

void ProcessLine(const std::string &line);
void SourceFile(const std::string &filepath);

inline std::string cwd;

// A command's arguments
inline std::vector<std::string> args;

}; // namespace Shell
