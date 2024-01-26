#pragma once

#include "libdeps.hpp"
#include "StandardStreams.hpp"

namespace NDS_Shell
{
	// Initializes necessary libnds resources.
	void Init();

	// Starts the prompt loop. Does not return.
	void Start();

	// Parses a shell line, and:
	// - If a command was parsed, populates `args` and `stdio`, and runs the specified command.
	// - If an envirionment variable assignment was parsed, performs the assignment.
	void ParseLine(const std::string &line);

	// Parse a double-quoted string. Returns false if no closing `"` was found.
	bool ParseDoubleQuotedString(std::string::const_iterator &itr, const std::string::const_iterator &lineEnd, std::string &currentArg);

	bool ParsePossibleRedirect(const std::string::const_iterator &startItr, const std::string::const_iterator &lineEnd, std::string &currentArg);
	bool ParseInputRedirect(const std::vector<int> fds, std::string::const_iterator &itr);
	bool ParseOutputRedirect(const std::vector<int> fds, std::string::const_iterator &itr);

	// A command's arguments
	inline std::vector<std::string> args;

	// Shell environment variables
	inline std::unordered_map<std::string, std::string> env {{"PS1", "> "}, {"CURSOR", "_"}};

	// A command's standard streams
	inline StandardStreams stdio;

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

		const std::unordered_map<std::string, void (*)()> map {
			{"help", help},
			{"exit", systemShutDown},
			{"clear", []() { *stdio.out << "\e[2J"; }},
			{"ls", ls},
			{"cat", cat},
			{"echo", echo},
			{"env", envCmd},
			{"dns", dns},
			{"wifi", wifi},
			{"http", http}
		};
	}
};