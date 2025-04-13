#pragma once

#include <netinet/in.h>

#include <string>

namespace NetUtils
{
enum class Error
{
	PORT_REQUIRED,
	PARSE_ERROR
};

extern Error error;

void printError(const std::string &prefix);
bool parseAddress(const char *addr, const int defaultPort, sockaddr_in &sain);
} // namespace NetUtils
