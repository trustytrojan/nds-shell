#include "CliPrompt.hpp"
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

#include <iostream>

void Commands::tcp()
{
	if (Shell::args.size() != 2)
	{
		std::cerr << "usage: http <ip:port>\n";
		return;
	}

	// parse the address
	sockaddr_in sain;
	if (!NetUtils::ParseAddress(Shell::args[1].c_str(), 80, sain))
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
	std::cerr << "tcp: socket: " << sock << '\n';

	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		close(sock);
		return;
	}
	std::cerr << "tcp: connected\n";

	CliPrompt prompt{"tcp> ", '_', std::cout};
	std::string lineToSend;

	fd_set master_set{};
	FD_SET(sock, &master_set);

	fd_set readfds{master_set};

	char buf[200]{};
	bool shouldExit{};
	prompt.resetProcessKeyboardState();
	std::cout << prompt.prompt;

	while (pmMainLoop() && !shouldExit)
	{
		swiWaitForVBlank();
		prompt.ProcessKeyboard(lineToSend);

		if (prompt.newlineWasEntered())
		{
			std::cout << "\r\e[1A\e[2K";
			switch (send(sock, lineToSend.c_str(), lineToSend.length(), 0))
			{
			case -1:
				std::cerr << "\e[41m";
				perror("send");
				std::cerr << "\e[39m";
				shouldExit = true;
				break;
			case 0:
				std::cout << "\r\e[2Ktcp: remote end disconnected\n";
				shouldExit = true;
				break;
			}
			prompt.resetProcessKeyboardState();
			lineToSend.clear();
			std::cout << prompt.prompt;
		}

		// Check for incoming data
		readfds = master_set;
		static timeval timeout{};
		const auto selectResult =
			select(sock + 1, &readfds, nullptr, nullptr, &timeout);

		if (selectResult > 0 && FD_ISSET(sock, &readfds))
		{
			switch (const auto bytesRead = recv(sock, buf, sizeof(buf) - 1, 0))
			{
			case -1:
				perror("recv");
				shouldExit = true;
				break;
			case 0:
				std::cout << "\r\e[2Ktcp: remote end disconnected\n";
				shouldExit = true;
				break;
			default:
				buf[bytesRead] = '\0';
				std::cout << "\r\e[2K" << buf << '\n'
						  << prompt.prompt << lineToSend;
				break;
			}
		}
		else if (selectResult == -1)
		{
			perror("select");
			shouldExit = true;
		}
	}

	close(sock);
}
