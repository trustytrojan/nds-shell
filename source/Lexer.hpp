#pragma once

#include "libdeps.hpp"

using StringIterator = std::string::const_iterator;
using EnvMap = std::unordered_map<std::string, std::string>;

struct Token
{
	enum class Type
	{
		STRING = 'S',
		INPUT_REDIRECT = '<',
		OUTPUT_REDIRECT = '>',
		PIPE = '|',
		OR = 'O',
		AND = 'A',
		SEMICOLON = ';'
	};

	const Type type;
	const std::string value;
};

bool LexLine(std::vector<Token> &tokens, const std::string &line, EnvMap &env);
