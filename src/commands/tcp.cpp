#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "NetUtils.hpp"

#include <dswifi9.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <iostream>

void Commands::tcp(const Context &ctx)
{
	if (ctx.args.size() != 2)
	{
		ctx.err << "usage: tcp <ip:port>\n";
		return;
	}

	const auto debugMessages = ctx.env.contains("TCP_DEBUG");

	// parse the address
	sockaddr_in sain;
	if (const auto rc = NetUtils::ParseAddress(sain, ctx.args[1]); rc != NetUtils::Error::NO_ERROR)
	{
		ctx.err << "\e[91mtcp: " << NetUtils::StrError(rc) << "\e[39m\n";
		return;
	}

	// open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		ctx.err << "\e[91mtcp: socket: " << strerror(errno) << "\e[39m\n";
		return;
	}

	if (debugMessages)
		ctx.err << "\e[90mtcp: socket: " << sock << "\n\e[39m";

	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		ctx.err << "\e[91mtcp: connect: " << strerror(errno) << "\e[39m\n";
		close(sock);
		return;
	}

	if (debugMessages)
		ctx.err << "\e[90mtcp: connected\e[39m\n";

	// TODO: get rid of the select() impl, use non-blocking sockets instead

	fd_set master_set{};
	FD_SET(sock, &master_set);

	fd_set readfds{master_set};

	char buf[200]{};
	bool shouldExit{};

	CliPrompt prompt;
	prompt.setOutputStream(ctx.out);
	prompt.setPrompt("tcp> ");
	prompt.prepareForNextLine();

	ctx.out << "press fold/esc key to exit\n";
	prompt.printFullPrompt(false);

	while (pmMainLoop() && !shouldExit)
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		if (ctx.shell.IsFocused())
		{
			// NOTE: we only need to prevent the PROMPT from processing the keyboard
			// when not focused! everything else is allowed to run!

			prompt.update();

			if (prompt.foldPressed())
			{
				if (debugMessages)
					ctx.out << "\r\e[2K\e[90mtcp: fold key pressed\e[39m\n";
				break;
			}

			if (prompt.enterPressed())
			{
				ctx.out << "\r\e[1A\e[2K";
				const auto &lineToSend = prompt.getInput();
				switch (send(sock, lineToSend.c_str(), lineToSend.length(), 0))
				{
				case -1:
					ctx.err << "\e[91mtcp: send: " << strerror(errno) << "\e[39m\n";
					shouldExit = true;
					break;
				case 0:
					ctx.out << "\r\e[2Ktcp: remote end disconnected\n";
					shouldExit = true;
					break;
				}
				prompt.prepareForNextLine();
				prompt.printFullPrompt(false);
			}
		}

		// Check for incoming data
		readfds = master_set;
		static timeval timeout{};
		const auto selectResult = select(sock + 1, &readfds, nullptr, nullptr, &timeout);

		if (selectResult > 0 && FD_ISSET(sock, &readfds))
		{
			switch (const auto bytesRead = recv(sock, buf, sizeof(buf) - 1, 0))
			{
			case -1:
				ctx.err << "\e[91mtcp: recv: " << strerror(errno) << "\e[39m\n";
				shouldExit = true;
				break;
			case 0:
				ctx.out << "\r\e[2Ktcp: remote end disconnected\n";
				shouldExit = true;
				break;
			default:
				buf[bytesRead] = '\0';
				ctx.out << "\r\e[2K" << buf << '\n';
				prompt.printFullPrompt(true);
				break;
			}
		}
		else if (selectResult == -1)
		{
			ctx.err << "\e[91mtcp: select: " << strerror(errno) << "\e[39m\n";
			shouldExit = true;
		}
	}

	// close() and closesocket() operate on DIFFERENT fd tables
	closesocket(sock);
}
