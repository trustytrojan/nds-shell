#ifndef __EVERYTHING__
#define __EVERYTHING__

#include <nds.h>
#include <fat.h>
#include <dswifi9.h>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <fstream>

typedef std::vector<std::string> Args;
typedef void (*CommandFunction)(const Args &);
typedef std::function<void(const std::string &)> ShellLineProcessor;

extern Keyboard *keyboard;

void StartShell(std::string prompt, const char cursorCharacter, const ShellLineProcessor lineProcessor, std::ostream &ostr);
void ProcessLine(const std::string &line);
void RunCommand(const Args &args);

#endif