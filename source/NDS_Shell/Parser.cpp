#include "../NDS_Shell.hpp"

using NDS_Shell::Lexer::Token;

bool NDS_Shell::Parser::ParseTokens(const std::vector<Token> &tokens)
{
	const auto tokensBegin = tokens.cbegin();
	const auto tokensEnd = tokens.cend();

	for (auto itr = tokensBegin; itr < tokensEnd; ++itr)
		// first, process all operators
		switch (itr->type)
		{
		case Token::Type::INPUT_REDIRECT:
			if (!ParseInputRedirect(itr, tokensBegin, tokensEnd))
				return false;
			break;
		
		case Token::Type::OUTPUT_REDIRECT:
			if (!ParseOutputRedirect(itr, tokensBegin, tokensEnd))
				return false;
			break;
		}

	return true;
}

bool NDS_Shell::Parser::ParseInputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
{
	const auto prevItr = itr - 1;
	const auto nextItr = itr + 1;

	if (nextItr == tokensEnd || nextItr->type != Token::Type::STRING)
	{
		std::cerr << "\e[41mshell: filename expected after `<`\e[39m\n";
		return false;
	}

	const auto &filename = nextItr->value;

	if (!std::filesystem::exists(filename))
	{
		std::cerr << "\e[41mshell: file `" << filename << "` does not exist\e[39m\n";
		return false;
	}

	// if `<` is the first token, or whitespace came before it
	if (itr == tokensBegin || prevItr->type == Token::Type::WHITESPACE)
	{
		// redirect the file to stdin
		RedirectInput(0, filename);
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	RedirectInput(atoi(fdStr.c_str()), filename);
	return true;
}

bool NDS_Shell::Parser::ParseOutputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
{
	const auto prevItr = itr - 1;
	const auto nextItr = itr + 1;

	if (nextItr == tokensEnd || nextItr->type != Token::Type::STRING)
	{
		std::cerr << "\e[41mshell: filename expected after `>`\e[39m\n";
		return false;
	}

	const auto &filename = nextItr->value;

	// if `<` is the first token, or whitespace came before it
	if (itr == tokensBegin || prevItr->type == Token::Type::WHITESPACE)
	{
		// redirect the file to stdin
		RedirectOutput(1, filename);
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	RedirectOutput(atoi(fdStr.c_str()), filename);
	return true;
}
