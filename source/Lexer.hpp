#pragma once

#include "libdeps.hpp"
#include "Shell.hpp"

using Shell::EnvMap;

struct Token
{
	enum class Type
	{
		WHITESPACE = 'W',
		STRING = 'S',
		INPUT_REDIRECT = '<',
		OUTPUT_REDIRECT = '>',
		PIPE = '|',
		OR = 'O',
		AND = 'A',
		SEMICOLON = ';',
		AMPERSAND = '&',
		EQUALS = '='
	};

	const Type type;
	const std::string value;
};

bool LexLine(std::vector<Token> &tokens, const std::string &line, EnvMap &env);
