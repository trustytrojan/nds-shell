#include "everything.h"

std::regex ipRegex(R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)");
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

sockaddr_in *parseAddress(const std::string &addr, const auto defaultPort)
{
	const auto colonIndex = addr.find(':');

	auto sain = new sockaddr_in;
	sain->sin_family = AF_INET;

	std::string ipOrHostname;

	if (colonIndex == std::string::npos)
	{
		sain->sin_port = htons(defaultPort);
		ipOrHostname = addr;
	}
	else
	{
		sain->sin_port = htons(std::stoi(addr.substr(colonIndex + 1)));
		ipOrHostname = addr.substr(0, colonIndex);
	}

	if (inet_aton(ipOrHostname))
}
