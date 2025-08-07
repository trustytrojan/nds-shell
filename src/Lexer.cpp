#include "Lexer.hpp"

#include <iostream>

using StrItr = std::string_view::const_iterator;

void EscapeInUnquotedString(StrItr &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial backslash.
	// Only escape spaces, backslashes, and equals.
	// Otherwise, omit the backslash.
	switch (*(++itr))
	{
	case ' ':
		currentToken += ' ';
		break;
	case '=':
		currentToken += '=';
		break;
	case '\\':
		currentToken += '\\';
		break;
	default:
		currentToken += *itr;
	}
}

void EscapeInDoubleQuoteString(StrItr &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial backslash.
	// Only escape backslashes, double-quotes, and dollar signs.
	switch (*(++itr))
	{
	case '"':
		currentToken += '"';
		break;
	case '\\':
		currentToken += '\\';
		break;
	case '$':
		currentToken += '$';
		break;
	default:
		currentToken += *itr;
	}
}

void InsertVariable(StrItr &itr, const StrItr &lineEnd, std::string &currentToken, const Env &env)
{
	// `itr` is pointing at the initial `$`
	// subtitute variables in the lexing phase to avoid the reiteration overhead
	// during parsing
	std::string varname;
	for (++itr; (isalnum(*itr) || *itr == '_') && *itr != '"' && itr < lineEnd; ++itr)
		varname += *itr;
	if (const auto envItr{env.find(varname)}; envItr != env.cend())
		currentToken += envItr->second;
	--itr; // Let the caller's loop see the character that stopped our loop,
		   // they might need it
}

bool LexDoubleQuoteString(StrItr &itr, const StrItr &lineEnd, std::string &currentToken, const Env &env)
{
	// When called, itr is pointing at the opening `"`, so increment before
	// looping.
	for (++itr; *itr != '"' && itr < lineEnd; ++itr)
		switch (*itr)
		{
		case '\\':
			EscapeInDoubleQuoteString(itr, currentToken);
			break;

		case '$':
			InsertVariable(itr, lineEnd, currentToken, env);
			break;

		default:
			currentToken += *itr;
		}

	if (itr == lineEnd)
	{
		std::cerr << "\e[91mshell: closing `\"` not found\e[39m\n";
		return false;
	}

	return true;
}

bool LexSingleQuoteString(StrItr &itr, const StrItr &lineEnd, std::string &currentToken)
{
	// Nothing can be escaped in single-quote strings.
	for (++itr; *itr != '\'' && itr < lineEnd; ++itr)
		currentToken += *itr;

	if (itr == lineEnd)
	{
		std::cerr << "\e[91mshell: closing `'` not found\e[39m\n";
		return false;
	}

	return true;
}

bool LexLine(std::vector<Token> &tokens, const std::string_view &line, const Env &env)
{
	tokens.clear();

	const auto lineEnd = line.cend();
	std::string currentToken;

	const auto pushAndClear = [&]
	{
		tokens.emplace_back(Token::Type::STRING, currentToken);
		currentToken.clear();
	};

	const auto pushAndClearIfNotEmpty = [&]
	{
		if (!currentToken.empty())
			pushAndClear();
	};

	for (auto itr = line.cbegin(); itr < lineEnd; ++itr)
	{
		if (isspace(*itr))
		{
			pushAndClearIfNotEmpty();
			if (tokens.back().type != Token::Type::WHITESPACE)
				tokens.emplace_back(Token::Type::WHITESPACE);
			continue;
		}

		switch (*itr)
		{
		case '\\':
			EscapeInUnquotedString(itr, currentToken);
			break;

		case '\'':
			if (!LexSingleQuoteString(itr, lineEnd, currentToken))
				return false;
			break;

		case '"':
			if (!LexDoubleQuoteString(itr, lineEnd, currentToken, env))
				return false;
			break;

		case '=':
			pushAndClearIfNotEmpty();
			tokens.emplace_back(Token::Type::EQUALS);
			break;

		case '$':
			// bash treats $1, $2, etc as special variables, we won't for now.
			InsertVariable(itr, lineEnd, currentToken, env);
			break;

		case ';':
			pushAndClearIfNotEmpty();
			tokens.emplace_back(Token::Type::SEMICOLON);
			break;

		case '<':
			pushAndClearIfNotEmpty();
			tokens.emplace_back(Token::Type::INPUT_REDIRECT);
			break;

		case '>':
			pushAndClearIfNotEmpty();
			tokens.emplace_back(Token::Type::OUTPUT_REDIRECT);
			break;

		case '|':
			pushAndClearIfNotEmpty();
			if (*(++itr) == '|')
				tokens.emplace_back(Token::Type::OR);
			else
			{
				tokens.emplace_back(Token::Type::PIPE);
				--itr;
			}
			break;

		case '&':
			pushAndClearIfNotEmpty();
			if (*(++itr) == '&')
				tokens.emplace_back(Token::Type::AND);
			else
			{
				tokens.emplace_back(Token::Type::AMPERSAND);
				--itr;
			}
			break;

		default:
			currentToken += *itr;
		}
	}

	if (!currentToken.empty())
		tokens.emplace_back(Token::Type::STRING, currentToken);

	return true;
}
