#include "NetUtils.hpp"

#include <netdb.h>

#include <string.h>
#include <sys/socket.h>

std::ostream &operator<<(std::ostream &ostr, const in_addr &ip)
{
	return ostr << (ip.s_addr & 0xFF) << '.' << ((ip.s_addr >> 8) & 0xFF) << '.' << ((ip.s_addr >> 16) & 0xFF) << '.'
				<< ((ip.s_addr >> 24) & 0xFF);
}

namespace NetUtils
{

const char *StrError(const Error error)
{
	switch (error)
	{
	case Error::NO_ERROR:
		return "success";
	case Error::PARSE_ERROR:
		return "parse error";
	case Error::PORT_REQUIRED:
		return "port required";
	}

	return "";
}

Error ParseAddress(sockaddr_in &sain, const char *addr, const int defaultPort)
{
	// ipv4
	sain.sin_family = AF_INET;

	// get port
	const auto colonPtr = strchr(addr, ':');
	if (!colonPtr)
	{
		if (defaultPort < 0)
			return Error::PORT_REQUIRED;

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
			return Error::PARSE_ERROR;

		sain.sin_addr = *(in_addr *)host->h_addr_list[0];
	}

	return Error::NO_ERROR;
}

Error ParseAddress(sockaddr_in &sain, const std::string &addr, const int defaultPort)
{
	return ParseAddress(sain, addr.c_str(), defaultPort);
}

} // namespace NetUtils
