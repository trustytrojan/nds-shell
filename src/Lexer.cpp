#include "Lexer.hpp"

#include <iostream>

using StrItr = std::string_view::const_iterator;

void EscapeInUnquotedString(StrItr &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial `\`.
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

void EscapeInDollarSignSinqleQuoteString(StrItr &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial `\`.
	switch (*(++itr))
	{
	case '"':
		currentToken += '"';
		break;
	case '\\':
		currentToken += '\\';
		break;
	case '$':
		// yes, for some odd reason, (ba)sh doesn't consume the `\` in this case.
		currentToken += "\\$";
		break;
	case 'n':
		currentToken += '\n';
		break;
	case 'e':
		currentToken += '\e';
		break;
	case 't':
		currentToken += '\t';
		break;
	case 'b':
		currentToken += '\b';
		break;
	case 'r':
		currentToken += '\r';
		break;
	}
}

void EscapeInDoubleQuoteString(StrItr &itr, std::string &currentToken)
{
	// `itr` is pointing at the initial `\`.
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
	// bash treats $1, $2, etc as special variables, we won't for now.
	// `itr` is pointing at the `$`.
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
	// itr is pointing at the opening `"`.
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

// NOTHING can be escaped in '' strings.
// don't believe me? run `echo '\'hi\''` and it will print `\hi'` go into "finish the string" mode.
bool LexSingleQuoteString(StrItr &itr, const StrItr &lineEnd, std::string &currentToken, bool withEscapes = false)
{
	// `itr` is pointing at the opening `'`.
	for (++itr; *itr != '\'' && itr < lineEnd; ++itr)
		currentToken += *itr;

	if (itr == lineEnd)
	{
		std::cerr << "\e[91mshell: closing `'` not found\e[39m\n";
		return false;
	}

	return true;
}

// this is for the $'' form of string literal. unlike "" and '', it will escape characters
// like newline (\n), escape (\e), tab (\t), backspace (\b), carriage return (\r) etc.
// like '', it does not perform variable substitution.
bool LexDollarSignSingleQuoteString(StrItr &itr, const StrItr &lineEnd, std::string &currentToken)
{
	// `itr` is pointing at the opening `'`.
	for (++itr; *itr != '\'' && itr < lineEnd; ++itr)
		switch (*itr)
		{
		case '\\':
			EscapeInDollarSignSinqleQuoteString(itr, currentToken);
			break;

		default:
			currentToken += *itr;
		}

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
		{
			const auto next = *(itr + 1);
			if (next == '\'')
				LexDollarSignSingleQuoteString(++itr, lineEnd, currentToken);
			else if (next == '"')
				// noticed this when playing around with ba(sh):
				// $"" strings are treated the same as "", so just consume the `$`.
				LexDoubleQuoteString(++itr, lineEnd, currentToken, env);
			else
				InsertVariable(itr, lineEnd, currentToken, env);
			break;
		}

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
