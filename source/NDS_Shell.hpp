#pragma once

#include "libdeps.hpp"
#include "Lexer.hpp"

namespace NDS_Shell
{
	using TokenIterator = std::vector<Token>::const_iterator;

	// Initializes necessary libnds resources.
	void Init();

	// Starts the prompt loop. Does not return.
	void Start();

	bool ParseTokens(const std::vector<Token> &token);
	bool ParseInputRedirect(TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd);
	bool RedirectInput(const int fd, const std::string &filename);

	// A command's arguments
	inline std::vector<std::string> args;

	// Shell environment variables
	inline std::unordered_map<std::string, std::string> env {{"PS1", "> "}, {"CURSOR", "_"}};

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
			{"clear", []() { std::cout << "\e[2J"; }},
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