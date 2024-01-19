#include "everything.h"

int netparseError;

void npPrintError(void)
{
	switch (netparseError)
	{
	case NP_PARSE_ERROR:
		std::cout << "Parse error\n";
		break;
	case NP_PORT_REQUIRED:
		std::cout << "Port required\n";
	}
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

	if (inet_aton(ipOrHostname.c_str(), &sain->sin_addr) <= 0) // string is an ip address
	{
		const auto host = gethostbyname(ipOrHostname.c_str()); // string is a domain name

		if (!host) // string is neither
		{
			delete sain;
			netparseError = NP_PARSE_ERROR;
			return NULL;
		}

		sain->sin_addr = *(in_addr *)host->h_addr_list[0];
	}

	return sain;
}
