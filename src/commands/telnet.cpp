#include "Commands.hpp"
#include "NetUtils.hpp"

#include <dswifi9.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <wfc.h>

#include <iostream>
#include <libtelnet.h>

static const telnet_telopt_t telopts[]{
	{TELNET_TELOPT_ECHO, TELNET_WONT, TELNET_DO}, {TELNET_TELOPT_TTYPE, TELNET_WILL, TELNET_DONT}, {-1, 0, 0}};

struct TelnetCtx
{
	int sock;
	const Commands::Context &shellCtx;
};

static void _send(int sock, const char *buffer, size_t size)
{
	int rs;

	/* send data */
	while (size > 0)
	{
		if ((rs = send(sock, buffer, size, 0)) == -1)
		{
			// This is non-blocking, so we might get EAGAIN
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				threadYield();
				continue;
			}
			// A real error occurred
			return;
		}
		else if (rs == 0)
		{
			// Should not happen with a non-blocking socket, but handle it.
			return;
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

static void _event_handler(telnet_t *telnet, telnet_event_t *ev, void *user_data)
{
	TelnetCtx *ctx = (TelnetCtx *)user_data;

	switch (ev->type)
	{
	/* data received */
	case TELNET_EV_DATA:
		for (const auto c : std::string_view{ev->data.buffer, ev->data.size})
			if (c == '\b')
				ctx->shellCtx.out << "\e[D";
			else
				ctx->shellCtx.out << c;
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(ctx->sock, ev->data.buffer, ev->data.size);
		break;
	/* request to enable remote feature (or receipt) */
	case TELNET_EV_WILL:
	case TELNET_EV_WONT:
	case TELNET_EV_DO:
	case TELNET_EV_DONT:
		// just acknowledge all negotiation, libtelnet handles this
		break;
	/* respond to TTYPE commands */
	case TELNET_EV_TTYPE:
		/* respond with our terminal type, if requested */
		if (ev->ttype.cmd == TELNET_TTYPE_SEND)
			telnet_ttype_is(telnet, "vt100");
		break;
	/* respond to particular subnegotiations */
	case TELNET_EV_SUBNEGOTIATION:
		break;
	/* error */
	case TELNET_EV_ERROR:
		ctx->shellCtx.err << "telnet error: " << ev->error.msg << '\n';
		break;
	default:
		/* ignore */
		break;
	}
}

void Commands::telnet(const Context &ctx)
{
	if (ctx.args.size() != 2)
	{
		ctx.err << "usage: telnet host[:port]\n";
		return;
	}

	const auto debugMessages = ctx.env.contains("DEBUG");

	// parse the address
	sockaddr_in sain;
	if (const auto rc = NetUtils::ParseAddress(sain, ctx.args[1], 23); rc != NetUtils::Error::NO_ERROR)
	{
		ctx.err << "\e[41mtelnet: " << NetUtils::StrError(rc) << "\e[39m\n";
		return;
	}

	// open a connection to the server
	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		ctx.err << "\e[41mtelnet: socket: " << strerror(errno) << "\e[39m\n";
		return;
	}

	if (debugMessages)
		ctx.err << "\e[40mtelnet: socket: " << sock << "\n\e[39m";

	bool connected{};
	while (pmMainLoop() && !connected)
	{
		threadYield();

		switch (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)))
		{
		case -1:
			if (errno == EINPROGRESS || errno == EALREADY)
				break;
			ctx.err << "\e[41mtelnet: connect: " << strerror(errno) << "\e[39m\n";
			closesocket(sock);
			return;
		case 0:
			connected = true;
		}
	}

	if (debugMessages)
		ctx.err << "\e[90mtelnet: connected\e[39m\n";

	int yes = 1;
	if (ioctl(sock, FIONBIO, &yes) == -1)
	{
		ctx.err << "\e[41mtelnet: ioctl: " << strerror(errno) << "\e[39m\n";
		closesocket(sock);
		return;
	}

	if (debugMessages)
		ctx.err << "\e[90mtelnet: ioctl succeeded\e[49m\n";

	ctx.out << "press fold/esc key to exit\n";

	TelnetCtx telnetCtx{sock, ctx};
	telnet_t *telnet = telnet_init(telopts, _event_handler, 0, &telnetCtx);

	bool shouldExit{};
	while (pmMainLoop() && !shouldExit)
	{
		threadYield();

		char buf[200]{};
		const auto bytesRead = recv(sock, buf, sizeof(buf), 0);
		if (bytesRead > 0)
		{
			telnet_recv(telnet, buf, bytesRead);
		}
		else if (bytesRead == 0)
		{
			ctx.err << "\e[91mtelnet: connection closed\e[39m\n";
			break;
		}
		else if (errno != EAGAIN && errno != EWOULDBLOCK)
		{
			ctx.err << "\e[41mtelnet: recv: " << strerror(errno) << "\e[39m\n";
			break;
		}

		if (!ctx.shell.IsFocused())
			continue;

		std::string seq;
		switch (const auto key = keyboardUpdate())
		{
		case DVK_FOLD:
			shouldExit = true;
			break;
		case DVK_ENTER:
			seq = "\r\n";
			break;
		case DVK_UP:
			seq = "\e[A";
			break;
		case DVK_DOWN:
			seq = "\e[B";
			break;
		case DVK_RIGHT:
			seq = "\e[C";
			break;
		case DVK_LEFT:
			seq = "\e[D";
			break;
		default:
			if (key > 0 && key < 128)
				seq = key;
			break;
		}

		if (!seq.empty())
			telnet_send(telnet, seq.c_str(), seq.length());
	}

	if (debugMessages)
		ctx.err << "\e[90mtelnet: after loop\e[39m\n";

	telnet_free(telnet);
	closesocket(sock);
}
