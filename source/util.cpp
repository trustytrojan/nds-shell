#include "everything.h"

std::vector<std::string> strsplit(const std::string &str, const char delim)
{
	if (str.empty())
		return std::vector<std::string>();
	std::istringstream iss(str);
	std::vector<std::string> tokens;
	std::string token;
	while (std::getline(iss, token, delim))
		tokens.push_back(token);
	return tokens;
}

void printWifiStatus(void)
{
	switch (Wifi_AssocStatus())
	{
	case ASSOCSTATUS_ASSOCIATED:
	{
		const auto ip = Wifi_GetIP();
		iprintf("Connected!\nIP: %li.%li.%li.%li", (ip) & 0xFF, (ip >> 8) & 0xFF, (ip >> 16) & 0xFF, (ip >> 24) & 0xFF);
		break;
	}
	case ASSOCSTATUS_ASSOCIATING:
		std::cout << "Associating...";
		break;
	case ASSOCSTATUS_AUTHENTICATING:
		std::cout << "Authenticating...";
		break;
	case ASSOCSTATUS_ACQUIRINGDHCP:
		std::cout << "Acquiring DHCP...";
		break;
	case ASSOCSTATUS_DISCONNECTED:
		std::cout << "Disconnected";
		break;
	case ASSOCSTATUS_SEARCHING:
		std::cout << "Searching...";
	}
	std::cout << '\n';
}