#include "Commands.hpp"
#include "CurlMulti.hpp"

#include <curl/curl.h>

#ifdef CURL_DEBUG
static int curl_debug(CURL *, curl_infotype type, char *const data, const size_t size, void *userp)
{
	if (type != CURLINFO_TEXT)
		// we only want debug messages, not req/resp data
		return 0;
	auto &ostr = *reinterpret_cast<std::ostream *>(userp);
	ostr << "\e[90m";
	ostr.write(data, size);
	ostr << "\e[39m";
	return 0;
}
#endif

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);

void loop(CURL *easy, const Commands::Context &ctx)
{
	// Begin send/recv loop.
	CliPrompt prompt{"", ctx.out};
	prompt.printFullPrompt(false);

	size_t sent;
	const curl_ws_frame *meta{};
	std::vector<char> msgbuf;
	while (pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		// Receive a frame, continue building msgbuf
		bool messageFullyReceived{};
		char buffer[1024];
		size_t rlen;
		switch (const auto res = curl_ws_recv(easy, buffer, sizeof(buffer), &rlen, &meta))
		{
		case CURLE_AGAIN:
			// didn't get anything, so just continue on
			break;
		case CURLE_OK:
			if (meta->flags & CURLWS_CLOSE)
				goto close;
			messageFullyReceived = !(meta->flags & CURLWS_CONT);
			msgbuf.insert(msgbuf.end(), buffer, buffer + rlen);
			break;
		default:
			ctx.err << "\e[91mws: curl_ws_recv: " << res << ": " << curl_easy_strerror(res) << "\n\e[39m";
			goto close;
		}

		// Check if msgbuf is complete
		if (messageFullyReceived)
		{
			// Let's finally print it out
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
			// to be 100% safe i will follow the official curl example
			const auto &input = prompt.getInput();
			auto buf = input.data();
			auto blen = input.size();
			while (blen && pmMainLoop())
			{
#ifdef NDSH_THREADING
				threadYield();
#else
				swiWaitForVBlank();
#endif
				switch (const auto res = curl_ws_send(easy, buf, blen, &sent, 0, CURLWS_TEXT))
				{
				case CURLE_AGAIN:
					break;
				case CURLE_OK:
					buf += sent;
					blen -= sent;
					break;
				default:
					ctx.err << "\e[91mws: curl_ws_send: " << res << ": " << curl_easy_strerror(res) << "\n\e[39m";
					goto close;
				}
			}
			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}
	}

close:
	curl_ws_send(easy, "", 0, &sent, 0, CURLWS_CLOSE);
	curl_easy_cleanup(easy);
}

void Commands::ws(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: ws <url>\n";
		return;
	}

	const auto easy = curl_easy_init();
	if (!easy)
	{
		ctx.err << "\e[91mws: curl_easy_init failed\e[39m\n";
		return;
	}

	curl_easy_setopt(easy, CURLOPT_URL, ctx.args[1].c_str());

	// REQUIRED to actually have a bidirectional WebSocket that we call curl_ws_send/recv on
	curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 2L);

	// set a timeout so we aren't stuck connecting/reading
	const auto timeout = atoi(ctx.GetEnv("CURL_TIMEOUT", "10").c_str());
	curl_easy_setopt(easy, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);

	// CA cert bundle file, set the filename using env
	// copy one from a linux system and it works flawlessly
	if (Shell::fsInitialized())
		curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
	else
		ctx.err << "\e[91mws: wss unavailable: fs not initialized\e[39m\n";

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	// note to self... DO NOT USE C++ LAMBDAS
	curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	// get curl errors
	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_errbuf);

#ifdef CURL_DEBUG
	// debug output
	if (ctx.env.contains("CURL_DEBUG"))
	{
		curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, curl_debug);
		curl_easy_setopt(easy, CURLOPT_DEBUGDATA, &ctx.err);
	}
#endif

	// This entire block of code below, regardless of NDSH_THREADING, simply connects to the WebSocket server.
	// **Only afterwards** can we begin using curl_ws_send and curl_ws_recv.
#ifdef NDSH_THREADING
	bool done{}, error{};

	CurlMulti::AddEasyHandle(
		easy,
		[&](CURLcode result, long respcode)
		{
			if (result != CURLE_OK)
			{
				ctx.err << "\e[91mws: connection failed: " << curl_easy_strerror(result) << ": " << curl_errbuf
						<< "\e[39m\n";
				error = true;
			}

			if (respcode != 101) // Switching Protocols
			{
				ctx.err << "\e[91ws: http response code is not 101\e[39m\n";
				error = true;
			}

			done = true;
		});

	ctx.out << "connecting...";
	while (!done && pmMainLoop())
		threadYield();
	ctx.out << "\r\e[2K";

	if (error)
		return;
#else
	if (const auto rc = curl_easy_perform(easy); rc != CURLE_OK)
	{
		ctx.err << "\e[91mcurl: " << curl_easy_strerror(rc) << ": " << curl_errbuf << "\e[39m\n";
		return;
	}
#endif

	loop(easy, ctx);
}
