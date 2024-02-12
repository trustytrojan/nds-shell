#pragma once

#include "libdeps.hpp"
#include "Lexer.hpp"

namespace NDS_Shell::Parser
{
	using TokenIterator = std::vector<Lexer::Token>::const_iterator;

	bool ParseTokens(const std::vector<Lexer::Token> &token);
	bool ParseInputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd);
	bool ParseOutputRedirect(const TokenIterator &itr, const TokenIterator &tokensBegin, const TokenIterator &tokensEnd);
}
