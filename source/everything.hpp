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
#include <algorithm>
#include <cmath>
#include <ranges>
#include <iomanip>

struct StandardStreams
{
	std::istream *in;
	std::ostream *out, *err;

	StandardStreams() : in(&std::cin), out(&std::cout), err(&std::cerr) {}
	StandardStreams(const StandardStreams &) = delete;
	StandardStreams &operator=(const StandardStreams &) = delete;
	~StandardStreams()
	{
		// if any stream isnt a standard stream,
		// we can safely assume it is a file stream.
		// remember std::(i/o)fstream and std::(i/o)stream are NOT the same size.
		// we MUST cast the pointers first, then delete them.

		if (in != &std::cin)
		{
			const auto fs = (std::ifstream *)in;
			fs->close();
			delete fs;
		}
		
		if (out != &std::cout)
		{
			const auto fs = (std::ofstream *)out;
			fs->close();
			delete fs;
		}

		if (err != &std::cerr)
		{
			const auto fs = (std::ofstream *)err;
			fs->close();
			delete fs;
		}
	}
};

typedef std::vector<std::string> Args;
typedef void (*CommandFunction)(const Args &, const StandardStreams &);
typedef std::function<void(const std::string &)> ShellLineProcessor;

extern Keyboard *keyboard;

void StartShell(std::string prompt, const char cursorCharacter, const ShellLineProcessor lineProcessor, std::ostream &ostr);
void ProcessLine(const std::string &line);
void RunCommand(const Args &args, const StandardStreams &stdio);

std::ostream &operator<<(std::ostream &ostr, const in_addr &ip);

#endif