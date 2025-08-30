#include "Commands.hpp"
#include "WebSocket.hpp"

#include <curl/curl.h>

void Commands::ws(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: ws <url>\n";
		return;
	}

	WebSocket ws{ctx.args[1]};
	ws.setConnectTimeout(atoi(ctx.GetEnv("CURL_TIMEOUT", "10").c_str()));
	ws.setCaFile(ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem"));

	if (const auto res = ws.connect(); res != CURLE_OK)
	{
		ctx.err << "\e[91mws: connection failed: " << curl_easy_strerror(res) << ": " << ws.getErrbuf() << "\e[39m\n";
		return;
	}

	// Begin send/recv loop.
	CliPrompt prompt{"", ctx.out};
	prompt.printFullPrompt(false);
	std::vector<char> msgbuf;

	while (pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif
		auto [rc, messageFullyReceived] = ws.recvFragment(msgbuf);
		if (messageFullyReceived)
		{
			for (const auto c : msgbuf)
				if (c == '\b')
					ctx.out << "\e[D";
				else
					ctx.out << c;
			msgbuf.clear();
		}

		// Lastly update & check the prompt for a line to send
		if (!ctx.shell.IsFocused())
			continue;

		prompt.update();

		if (prompt.foldPressed())
			break;

		if (prompt.enterPressed())
		{
			ws.send(prompt.getInput());
			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}
	}
}
