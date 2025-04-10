#include "Shell.hpp"
#include "Parser.hpp"

bool ParseInputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd);
bool ParseOutputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd);

bool ParseTokens(const std::vector<Token> &tokens)
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

bool ParseInputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
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
		Shell::RedirectInput(0, filename);
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	Shell::RedirectInput(atoi(fdStr.c_str()), filename);
	return true;
}

bool ParseOutputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
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
		Shell::RedirectOutput(1, filename);
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	Shell::RedirectOutput(atoi(fdStr.c_str()), filename);
	return true;
}
