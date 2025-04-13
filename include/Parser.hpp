#pragma once

#include "Lexer.hpp"

using TokenIterator = std::vector<Token>::const_iterator;

bool ParseTokens(const std::vector<Token> &token);
