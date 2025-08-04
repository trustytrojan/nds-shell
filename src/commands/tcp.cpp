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
		*Shell::err << "usage: tcp <ip:port>\n";
		return;
	}

	// parse the address
	sockaddr_in sain;
	if (!NetUtils::ParseAddress(Shell::args[1].c_str(), 80, sain))
	{
		NetUtils::PrintError("tcp");
		return;
	}

	// open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		return;
	}
	*Shell::err << "\e[40mtcp: socket: " << sock << "\n\e[39m";

	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		close(sock);
		return;
	}
	*Shell::err << "\e[40mtcp: connected\e[39m\n";

	fd_set master_set{};
	FD_SET(sock, &master_set);

	fd_set readfds{master_set};

	char buf[200]{};
	bool shouldExit{};

	CliPrompt prompt;
	prompt.setOutputStream(Shell::out);
	prompt.setPrompt("tcp> ");
	prompt.prepareForNextLine();

	*Shell::out << "press fold/esc key to exit\n";

	// *Shell::out << prompt.prompt << prompt.cursor
	// 			<< EscapeSequences::Cursor::moveLeftOne;
	prompt.printFullPrompt(false);

	while (pmMainLoop() && !shouldExit)
	{
		swiWaitForVBlank();
		prompt.processKeyboard();

		if (prompt.foldPressed())
		{
			*Shell::out << "\r\e[2Ktcp: fold key pressed\n";
			break;
		}

		if (prompt.enterPressed())
		{
			*Shell::out << "\r\e[1A\e[2K";
			const auto &lineToSend = prompt.getInput();
			switch (send(sock, lineToSend.c_str(), lineToSend.length(), 0))
			{
			case -1:
				*Shell::err << "\e[41m";
				perror("send");
				*Shell::err << "\e[39m";
				shouldExit = true;
				break;
			case 0:
				*Shell::out << "\r\e[2Ktcp: remote end disconnected\n";
				shouldExit = true;
				break;
			}
			prompt.prepareForNextLine();
			// lineToSend.clear();
			// *Shell::out << prompt.prompt << prompt.cursor
			// 			<< EscapeSequences::Cursor::moveLeftOne;
			prompt.printFullPrompt(false);
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
				*Shell::out << "\r\e[2Ktcp: remote end disconnected\n";
				shouldExit = true;
				break;
			default:
				buf[bytesRead] = '\0';
				*Shell::out << "\r\e[2K" << buf << '\n';
				// << prompt.prompt << lineToSend << prompt.cursor;
				prompt.printFullPrompt(true);
				break;
			}
		}
		else if (selectResult == -1)
		{
			perror("select");
			shouldExit = true;
		}
	}

	// close() and closesocket() operate on DIFFERENT fd tables
	closesocket(sock);
}
