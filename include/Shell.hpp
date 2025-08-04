#pragma once

#include "CliPrompt.hpp"

#include <nds.h>

#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

using Env = std::unordered_map<std::string, std::string>;

namespace Shell
{

const int NUM_CONSOLES = 7;
inline PrintConsole consoles[NUM_CONSOLES];
inline int focused_console{}, focused_display{};
void waitUntilKeysPressed(int keys);

void InitConsole();
void InitMultiConsole();

inline std::ofstream con_streams[NUM_CONSOLES];

// Initializes optional resources (fat, wifi).
void InitResources();

// Starts the prompt loop. Does not return.
void Start();

void ProcessLine(const std::string &line);
void SourceFile(const std::string &filepath);
void RedirectOutput(int fd, const std::string &filename);
void RedirectInput(int fd, const std::string &filename);
void ResetStreams();

bool HasEnv(const std::string &key);
std::optional<std::string> GetEnv(const std::string &key);
std::string GetEnv(const std::string &key, const std::string &_default);

inline std::string cwd;
inline Env env, commandEnv;
inline std::ofstream outf, errf;
inline std::ifstream inf;
inline std::ostream *out{&std::cout}, *err{&std::cerr};
inline std::istream *in{&std::cin};

// A command's arguments
inline std::vector<std::string> args;

}; // namespace Shell
