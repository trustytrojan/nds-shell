#include "everything.h"

#define NP_PORT_REQUIRED 1
#define NP_PARSE_ERROR 2

int netparseError;

sockaddr_in *parseIpAddress(const std::string &addr)
{
	const auto colonIndex = addr.find(':');

	if (colonIndex == std::string::npos)
		return NULL;

	auto sain = new sockaddr_in;
	sain->sin_family = AF_INET;
	sain->sin_port = htons(std::stoi(addr.substr(colonIndex + 1)));

	if (inet_aton(addr.substr(0, colonIndex).c_str(), &(sain->sin_addr)) <= 0)
	{
		delete sain;
		return NULL;
	}

	return sain;
}

sockaddr_in *parseAddress(const std::string &addr, const int defaultPort)
{
	const auto colonIndex = addr.find(':');

	auto sain = new sockaddr_in;
	sain->sin_family = AF_INET;

	std::string ipOrHostname;

	if (colonIndex == std::string::npos)
	{
		if (defaultPort < 0)
		{
			delete sain;
			netparseError = NP_PORT_REQUIRED;
			return NULL;
		}

		sain->sin_port = htons(defaultPort);
		ipOrHostname = addr;
	}
	else
	{
		sain->sin_port = htons(std::stoi(addr.substr(colonIndex + 1)));
		ipOrHostname = addr.substr(0, colonIndex);
	}

	if (inet_aton(ipOrHostname.c_str(), &sain->sin_addr) <= 0)
	{
		const auto host = gethostbyname(ipOrHostname.c_str());

		if (!host)
		{
			delete sain;
			netparseError = NP_PARSE_ERROR;
			return NULL;
		}

		sain->sin_addr = *(in_addr *)host->h_addr_list[0];
	}

	return sain;
}
