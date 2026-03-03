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

	auto url = ctx.args[1];

	if (!url.starts_with("ws://") && !url.starts_with("wss://"))
	{
		ctx.err << "\e[91murl protocol must be ws:// or wss://\e[39m\n";
		return;
	}

	WebSocket ws{ctx.args[1]};
	ws.setConnectTimeout(atoi(ctx.GetEnv("CURL_TIMEOUT", "10").c_str()));
	ws.setCaFile(ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem"));

	ws.on_message(
		[&](auto &msg)
		{
			for (const auto c : msg)
				if (c == '\b')
					ctx.out << "\e[D";
				else
					ctx.out << c;
		});

	ws.on_error([&](auto code, auto msg)
				{ ctx.err << "\e[91mws: " << curl_easy_strerror(code) << ": " << msg << "\e[m\n"; });

	bool closed{};
	ws.on_close([&](auto, auto) { closed = true; });

	if (const auto res = ws.connect(); res != CURLE_OK)
	{
		ctx.err << "\e[91mws: connect: " << curl_easy_strerror(res) << "\e[m\n";
		return;
	}

	// Begin send/recv loop.
	CliPrompt prompt{"", ctx.out};
	prompt.printFullPrompt(false);

	while (!closed && pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		// Poll the websocket for events
		ws.poll();

#ifdef NDSH_MULTICONSOLE
		// Lastly update & check the prompt for a line to send
		if (!ctx.shell.IsFocused())
			continue;
#endif

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
