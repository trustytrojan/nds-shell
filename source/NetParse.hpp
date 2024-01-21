#ifndef __NETPARSE__
#define __NETPARSE__

#include "everything.hpp"

namespace NetParse
{
	enum class Error
	{
		PORT_REQUIRED,
		PARSE_ERROR
	};

	extern Error error;

	void printError(const std::string &prefix);
	bool parseAddress(const char *addr, const int defaultPort, sockaddr_in &sain);
}

#endif