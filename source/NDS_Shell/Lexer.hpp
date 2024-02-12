#pragma once

#include "libdeps.hpp"

namespace NDS_Shell::Lexer
{
	using StringIterator = std::string::const_iterator;
	using EnvMap = std::unordered_map<std::string, std::string>;

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

	void EscapeInUnquotedString(StringIterator &itr, std::string &currentToken);
	void EscapeInDoubleQuoteString(StringIterator &itr, std::string &currentToken);
	void InsertVariable(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken, EnvMap &env);
	bool LexDoubleQuoteString(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken, EnvMap &env);
	bool LexSingleQuoteString(StringIterator &itr, const StringIterator &lineEnd, std::string &currentToken);
	bool LexLine(std::vector<Token> &tokens, const std::string &line, EnvMap &env);
}
