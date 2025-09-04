#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "NetUtils.hpp"

#include <dswifi9.h>
#include <sys/ioctl.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <expected>
#include <iostream>

struct TcpSocket
{
	const int fd{socket(AF_INET, SOCK_STREAM, 0)};
	~TcpSocket() { closesocket(fd); }

	// direct system call wrappers
	int connect(sockaddr_in &sain) { return ::connect(fd, (sockaddr *)&sain, sizeof(sockaddr_in)); }
	int recv(void *data, size_t recvlength) { return ::recv(fd, data, recvlength, 0); }
	int send(const void *data, size_t sendlength) { return ::send(fd, data, sendlength, 0); }

	std::expected<void, const char *> connect(const std::string_view &hostport)
	{
		sockaddr_in sain;
		if (const auto rc = NetUtils::ParseAddress(sain, hostport.data()); rc != NetUtils::Error::NO_ERROR)
			return std::unexpected{NetUtils::StrError(rc)};
		if (connect(sain) == -1)
			return std::unexpected{strerror(errno)};
		return {};
	}

	int send(const std::string_view &s) { return send(s.data(), s.size()); }

	std::expected<void, const char *> setNonblocking(bool yes)
	{
		if (::ioctl(fd, FIONBIO, &yes) == -1)
			return std::unexpected{strerror(errno)};
		return {};
	}
};

void Commands::tcp(const Context &ctx)
{
	if (ctx.args.size() != 2)
	{
		ctx.err << "usage: tcp <ip:port>\n";
		return;
	}

	const auto debugPrint = ctx.env.contains("TCP_DEBUG");

	TcpSocket sock;

	if (debugPrint)
		ctx.err << "\e[90mtcp: socket: " << sock.fd << "\n\e[39m";

	if (const auto ex = sock.connect(ctx.args[1]); !ex)
	{
		ctx.err << "\e[91mtcp: connect: " << ex.error() << "\e[39m\n";
		return;
	}

	if (debugPrint)
		ctx.err << "\e[90mtcp: connected\e[39m\n";

	if (const auto ex = sock.setNonblocking(true); !ex)
	{
		ctx.err << "\e[91mtelnet: ioctl: " << ex.error() << "\e[39m\n";
		return;
	}

	if (debugPrint)
		ctx.err << "\e[90mtelnet: set nonblocking\e[39m\n";

	char buf[200]{};
	bool shouldExit{};

	CliPrompt prompt;
	prompt.setOutputStream(ctx.out);
	prompt.setPrompt("");
	prompt.prepareForNextLine();
	ctx.out << "press fold/esc key to exit\n";

	while (pmMainLoop() && !shouldExit)
	{
		switch (const auto bytesRead = sock.recv(buf, sizeof(buf) - 1))
		{
		case -1:
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				// nothing came in
				break;
			ctx.err << "\e[91mtcp: recv: " << strerror(errno) << "\e[39m\n";
			shouldExit = true;
			break;
		case 0:
			ctx.out << "tcp: remote end disconnected\n";
			shouldExit = true;
			break;
		default:
			buf[bytesRead] = '\0';
			ctx.out << buf << '\n';
			break;
		}

		if (!ctx.shell.IsFocused())
			continue;

		prompt.update();

		if (prompt.foldPressed())
		{
			if (debugPrint)
				ctx.out << "\e[90mtcp: fold key pressed\e[39m\n";
			break;
		}

		if (!prompt.enterPressed())
			continue;

		switch (sock.send(prompt.getInput()))
		{
		case -1:
			ctx.err << "\e[91mtcp: send: " << strerror(errno) << "\e[39m\n";
			shouldExit = true;
			break;
		case 0:
			ctx.out << "tcp: remote end disconnected\n";
			shouldExit = true;
			break;
		}

		prompt.prepareForNextLine();
	}
}
