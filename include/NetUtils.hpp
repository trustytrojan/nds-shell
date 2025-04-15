#pragma once

#include <netinet/in.h>

#include <string>
#include <iostream>

namespace NetUtils
{

	enum class Error
{
	PORT_REQUIRED,
	PARSE_ERROR
};

extern Error error;

void PrintError(const std::string &prefix);
bool ParseAddress(const char *addr, const int defaultPort, sockaddr_in &sain);

} // namespace NetUtils

// for printing ip addresses without first converting to a string
inline std::ostream &operator<<(std::ostream &ostr, const in_addr &ip)
{
	return ostr << (ip.s_addr & 0xFF) << '.' << ((ip.s_addr >> 8) & 0xFF) << '.'
				<< ((ip.s_addr >> 16) & 0xFF) << '.'
				<< ((ip.s_addr >> 24) & 0xFF);
}
