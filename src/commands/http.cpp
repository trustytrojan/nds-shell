#include "Commands.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <unordered_set>

void Commands::http()
{
	if (Shell::args.size() != 3)
	{
		std::cerr << "usage: http <method> <url>\n";
		return;
	}

	// convert to uppercase
	auto method = Shell::args[1];
	std::ranges::transform(method, method.begin(), toupper);

	// check against supported methods
	static const std::unordered_set<std::string> httpMethods{
		"GET", "POST", "PUT", "DELETE"};
	if (std::find(httpMethods.begin(), httpMethods.cend(), method) ==
		httpMethods.cend())
	{
		std::cerr << "\e[41mhttp: invalid method\e[39m\n";
		return;
	}

	// parse out address and path from url
	// the string.h functions are easier to use for this kind of stuff
	auto addr = Shell::args[2].c_str();

	// take out http:// from url if its there
	if (!strncmp(addr, "http://", 7))
		addr += 7;

	const char *path;
	const auto slashPtr = strchr(addr, '/');
	if (!slashPtr)
		path = "/";
	else
	{
		// we lose the / here, but we're saving what could be a long copy
		// we can insert a / back when constructing the request, see below
		*slashPtr = 0;
		path = slashPtr + 1;
	}

	// parse the address
	sockaddr_in sain;
	if (!NetUtils::ParseAddress(addr, 80, sain))
	{
		NetUtils::PrintError("http");
		return;
	}

	// open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		return;
	}
	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	// construct and send the request
	std::stringstream ss;
	ss << method << ' ' << (slashPtr ? "/" : "") << path
	   << " HTTP/1.1\r\nHost: " << addr
	   << "\r\nUser-Agent: Nintendo DS\r\n\r\n";
	const auto request = ss.str();
	if (send(sock, request.c_str(), request.size(), 0) == -1)
	{
		perror("send");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	// print the response
	char responseBuffer[BUFSIZ + 1];
	responseBuffer[BUFSIZ] = 0;
	int bytesReceived;
	while ((bytesReceived = recv(sock, responseBuffer, BUFSIZ, 0)) > 0)
	{
		responseBuffer[bytesReceived] = 0;
		std::cout << responseBuffer;
	}

	if (bytesReceived == -1)
	{
		perror("recv");
		if (close(sock) == -1)
			perror("close");
		return;
	}

	if (close(sock) == -1)
		perror("close");

	std::cerr << "\e[45mreached end of function\e[39m\n";
}
