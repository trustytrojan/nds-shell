#pragma once

#include <nds.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Env = std::unordered_map<std::string, std::string>;

namespace Shell
{

// Initializes necessary libnds resources.
void Init();

// Starts the prompt loop. Does not return.
void Start();

void ProcessLine(const std::string &line);
void SourceFile(const std::string &filepath);
void RedirectOutput(int fd, const std::string &filename);
void RedirectInput(int fd, const std::string &filename);
void ResetStreams();

inline std::string cwd;
inline Env env, commandEnv;
inline std::ofstream outf, errf;
inline std::ifstream inf;
inline std::ostream *out{&std::cout}, *err{&std::cerr};
inline std::istream *in{&std::cin};

// A command's arguments
inline std::vector<std::string> args;

}; // namespace Shell
