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

void Commands::http()
{
	if (Shell::args.size() != 3)
	{
		std::cerr << "usage: http <method> <url>\n";
		return;
	}

	// Convert to uppercase
	auto method = Shell::args[1];
	std::ranges::transform(method, method.begin(), toupper);

	// Parse out address and path from url
	auto addr = Shell::args[2].c_str();

	// Take out http:// from url if it's there
	if (!strncmp(addr, "http://", 7))
		addr += 7;

	const char *path;
	const auto slashPtr = strchr(addr, '/');
	if (!slashPtr)
		path = "/";
	else
	{
		*slashPtr = 0;
		path = slashPtr + 1;
	}

	// Parse the address
	sockaddr_in sain;
	if (!NetUtils::ParseAddress(addr, 80, sain))
	{
		NetUtils::PrintError("http");
		return;
	}

	// Open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		return;
	}
	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		closesocket(sock);
		return;
	}

	// Construct and send the request
	std::stringstream ss;
	ss << method << ' ' << (slashPtr ? "/" : "") << path
	   << " HTTP/1.1\r\nHost: " << addr
	   << "\r\nUser-Agent: Nintendo DS\r\nConnection: close\r\n\r\n"; // Force
																	  // connection
																	  // close
	const auto request = ss.str();
	if (send(sock, request.c_str(), request.size(), 0) == -1)
	{
		perror("send");
		closesocket(sock);
		return;
	}

	// Read response
	char buffer[1024];
	int totalReceived = 0;
	int contentLength = -1;
	bool headersComplete = false;
	std::string headers;

	// First pass: read headers
	while (!headersComplete)
	{
		int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
		if (bytesReceived <= 0)
		{
			perror("recv");
			closesocket(sock);
			return;
		}

		totalReceived += bytesReceived;
		headers.append(buffer, bytesReceived);

		// Check for end of headers
		size_t headerEnd = headers.find("\r\n\r\n");
		if (headerEnd != std::string::npos)
		{
			headersComplete = true;

			// Find Content-Length header
			size_t clPos = headers.find("Content-Length: ");
			if (clPos != std::string::npos)
				contentLength = atoi(headers.c_str() + clPos + 16);

			// Print headers
			std::cout << headers.substr(0, headerEnd + 4);

			// Handle any body data we might have already read
			if (headers.size() > headerEnd + 4)
			{
				std::cout.write(
					headers.c_str() + headerEnd + 4,
					headers.size() - (headerEnd + 4));
				if (contentLength > 0)
					contentLength -= (headers.size() - (headerEnd + 4));
			}
		}
	}

	// Second pass: read body if we know the length
	if (contentLength > 0)
	{
		while (contentLength > 0)
		{
			int bytesReceived = recv(
				sock, buffer, std::min(contentLength, (int)sizeof(buffer)), 0);
			if (bytesReceived <= 0)
				break;

			std::cout.write(buffer, bytesReceived);
			contentLength -= bytesReceived;
		}
	}
	else
	{
		// No Content-Length - read until connection closes
		while (true)
		{
			int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
			if (bytesReceived <= 0)
				break;
			std::cout.write(buffer, bytesReceived);
		}
	}

	closesocket(sock);
}
