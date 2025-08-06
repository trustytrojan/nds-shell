#include "Parser.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

bool ParseInputRedirect(
	TokenIterator &itr, IoRedirect &ior, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
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
		ior = {0, IoRedirect::Direction::IN, filename};
		++itr; // increment to the filename token, so the main loop increments
			   // past *that*
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	++itr; // increment to the filename token, so the main loop increments
		   // past *that*

	ior = {atoi(fdStr.c_str()), IoRedirect::Direction::IN, filename};
	return true;
}

bool ParseOutputRedirect(
	TokenIterator &itr, IoRedirect &ior, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd)
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
		// redirect stdout to the file
		ior = {1, IoRedirect::Direction::OUT, filename};
		++itr; // increment to the filename token, so the main loop increments
			   // past *that*
		return true;
	}

	const auto &fdStr = prevItr->value;

	if (prevItr->type != Token::Type::STRING || !std::ranges::all_of(fdStr, isdigit))
	{
		std::cerr << "\e[41mshell: integer expected before `<`\e[39m\n";
		return false;
	}

	++itr; // increment to the filename token, so the main loop increments
		   // past *that*

	ior = {atoi(fdStr.c_str()), IoRedirect::Direction::OUT, filename};
	return true;
}

bool ParseTokens(
	const std::vector<Token> &tokens,
	std::vector<IoRedirect> &redirects,
	std::vector<EnvAssign> &envAssigns,
	std::vector<std::string> &args)
{
	const auto tokensBegin{tokens.cbegin()}, tokensEnd{tokens.cend()};
	bool firstArgPlaced{};

	// First pass: collect all redirections
	for (auto itr = tokensBegin; itr < tokensEnd; ++itr)
	{
		if (itr->type == Token::Type::INPUT_REDIRECT)
		{
			IoRedirect ior;
			if (!ParseInputRedirect(itr, ior, tokensBegin, tokensEnd))
				return false;
			redirects.push_back(ior);
		}
		else if (itr->type == Token::Type::OUTPUT_REDIRECT)
		{
			IoRedirect ior;
			if (!ParseOutputRedirect(itr, ior, tokensBegin, tokensEnd))
				return false;
			redirects.push_back(ior);
		}
	}

	// Second pass: collect env assignments and command arguments
	for (auto itr = tokensBegin; itr < tokensEnd; ++itr)
	{
		switch (itr->type)
		{
		case Token::Type::STRING:
			// Check if this is an env assignment (KEY=VALUE before first
			// argument)
			// clang-format off
			if (!firstArgPlaced &&
				(itr + 1) < tokensEnd &&
				(itr + 1)->type == Token::Type::EQUALS &&
				(itr + 2) < tokensEnd &&
				(itr + 2)->type == Token::Type::STRING)
			// clang-format on
			{
				envAssigns.emplace_back(itr->value, (itr + 2)->value);
				itr += 2; // Skip past the = and value
			}
			else
			{
				args.emplace_back(itr->value);
				firstArgPlaced = true;
			}
			break;

		case Token::Type::INPUT_REDIRECT:
		case Token::Type::OUTPUT_REDIRECT:
			// Skip the filename token too (already processed in first pass)
			++itr;
			break;

		case Token::Type::WHITESPACE:
		case Token::Type::PIPE:
		case Token::Type::OR:
		case Token::Type::AND:
		case Token::Type::SEMICOLON:
		case Token::Type::AMPERSAND:
		case Token::Type::EQUALS:
			// Ignore these for now
			break;
		}
	}

	return true;
}
