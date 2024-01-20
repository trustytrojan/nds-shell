#include "NetParse.hpp"

using namespace NetParse;

Error NetParse::error;

void NetParse::printError(const std::string &s)
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

bool NetParse::parseAddress(const std::string &addr, const int defaultPort, sockaddr_in &sain)
{
	sain.sin_family = AF_INET;
	const auto colonIndex = addr.find(':');
	std::string ipOrHostname;

	if (colonIndex == std::string::npos)
	{
		if (defaultPort < 0)
		{
			error = Error::PORT_REQUIRED;
			return false;
		}

		sain.sin_port = htons(defaultPort);
		ipOrHostname = addr;
	}
	else
	{
		sain.sin_port = htons(std::stoi(addr.substr(colonIndex + 1)));
		ipOrHostname = addr.substr(0, colonIndex);
	}

	if (inet_aton(ipOrHostname.c_str(), &sain.sin_addr) <= 0) // string is an ip address
	{
		const auto host = gethostbyname(ipOrHostname.c_str()); // string is a domain name

		if (!host) // string is neither
		{
			error = Error::PARSE_ERROR;
			return false;
		}

		sain.sin_addr = *(in_addr *)host->h_addr_list[0];
	}

	return true;
}
