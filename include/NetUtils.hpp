#pragma once

#include <netinet/in.h>

#include <iostream>

namespace NetUtils
{

enum class Error
{
	NO_ERROR,
	PORT_REQUIRED,
	PARSE_ERROR
};

const char *StrError(Error e);

// Parse an address in the form `host:port`.
// Domain names will be resolved using `gethostbyname()`.
// If `defaultPort` is `-1`, the `:port` part of `addr` is required.
Error ParseAddress(sockaddr_in &sain, const char *addr, const int defaultPort = -1);
Error ParseAddress(sockaddr_in &sain, const std::string &addr, const int defaultPort = -1);

} // namespace NetUtils

// for printing ip addresses without first converting to a string
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip);
