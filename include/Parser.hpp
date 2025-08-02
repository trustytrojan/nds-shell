#pragma once

#include "Lexer.hpp"

using TokenIterator = std::vector<Token>::const_iterator;

/*
$ cat 1<echofile
k
cat: write error: Bad file descriptor
$ cat 1<echofile 1>catfile
d
^C

The rightmost redirect overrides all other redirects USING THE SAME FD.
*/
struct IoRedirect
{
	enum class Direction
	{
		IN,
		OUT
	};

	int fd;
	Direction direction;
	std::string filename;
};

/*
$ X=6 bash -c 'echo $X $Y' Y=7
6

Command-local env assignments are only recorded up to the
first command argument (executable/command name). After that,
they are treated as strings.
*/
struct EnvAssign
{
	std::string key, value;
};

// maybe let's not support pipelines yet, as this has concurrency considerations
// it would already be a feat to get the rest working first

bool ParseTokens(
	const std::vector<Token> &tokens,
	std::vector<IoRedirect> &redirects,
	std::vector<EnvAssign> &envAssigns,
	std::vector<std::string> &args);
