#include "NetUtils.hpp"

using namespace NetUtils;

Error NetUtils::error;

void NetUtils::printError(const std::string &s)
{
	std::cerr << s << ": ";
	switch (error)
	{
	case Error::PARSE_ERROR:
		std::cerr << "parse error";
		break;
	case Error::PORT_REQUIRED:
		std::cerr << "port required";
	}
	std::cerr << '\n';
}

bool NetUtils::parseAddress(const char *addr, const int defaultPort, sockaddr_in &sain)
{
	// ipv4
	sain.sin_family = AF_INET;

	// get port
	const auto colonPtr = strchr(addr, ':');
	if (!colonPtr)
	{
		if (defaultPort < 0)
		{
			error = Error::PORT_REQUIRED;
			return false;
		}

		sain.sin_port = htons(defaultPort);
	}
	else
	{
		*colonPtr = 0;
		sain.sin_port = htons(atoi(colonPtr + 1));
	}

	// get address
	if (inet_aton(addr, &sain.sin_addr) <= 0)
	{
		const auto host = gethostbyname(addr);

		if (!host)
		{
			error = Error::PARSE_ERROR;
			return false;
		}

		sain.sin_addr = *(in_addr *)host->h_addr_list[0];
	}

	return true;
}
