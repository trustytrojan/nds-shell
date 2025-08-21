#include "Commands.hpp"
#include "CurlMulti.hpp"

#include <curl/curl.h>

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);

// WebSocket constants (in case they're not defined)
#ifndef CURLWS_TEXT
#define CURLWS_TEXT (1<<0)
#endif
#ifndef CURLWS_BINARY
#define CURLWS_BINARY (1<<1)
#endif
#ifndef CURLWS_PING
#define CURLWS_PING (1<<2)
#endif
#ifndef CURLWS_PONG
#define CURLWS_PONG (1<<3)
#endif
#ifndef CURLWS_CLOSE
#define CURLWS_CLOSE (1<<4)
#endif

void Commands::ws(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: ws <url> [message]\n";
		ctx.err << "  Establish WebSocket connection and optionally send message\n";
		ctx.err << "  Examples:\n";
		ctx.err << "    ws wss://gateway.discord.gg/?v=10&encoding=json\n";
		ctx.err << "    ws ws://echo.websocket.org \"Hello WebSocket\"\n";
		return;
	}

	const auto easy = curl_easy_init();
	if (!easy)
	{
		ctx.err << "\e[91mws: curl_easy_init failed\e[39m\n";
		return;
	}

	curl_easy_setopt(easy, CURLOPT_URL, ctx.args[1].c_str());

	// Try to set up WebSocket connection using CONNECT_ONLY
	// If WebSocket is not supported, this will fall back to basic connection
	curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 2L); // 2 for WebSocket if supported, otherwise 1

	// Use custom opensocket callback for NDS compatibility
	curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	// Set timeout
	const auto timeout = atoi(ctx.GetEnv("WS_TIMEOUT", "10").c_str());
	curl_easy_setopt(easy, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);

	// CA cert bundle for WSS
	if (Shell::fsInitialized())
		curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
	else if (ctx.args[1].find("wss://") == 0)
		ctx.err << "\e[91mws: wss unavailable: fs not initialized\e[39m\n";

	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_errbuf);

#ifdef NDSH_THREADING
	bool done = false;
	CurlMulti::AddEasyHandle(
		easy,
		[&](CURLcode result, long response_code)
		{
			if (result != CURLE_OK)
			{
				ctx.err << "\e[91mws: " << curl_easy_strerror(result) << ": " << curl_errbuf << "\e[39m\n";
			}
			else
			{
				ctx.out << "WebSocket connection established to " << ctx.args[1] << "\n";
				
				// If a message was provided, try to send it
				if (ctx.args.size() > 2)
				{
					const auto& message = ctx.args[2];
					
					// Try using curl_ws_send if available, otherwise note limitation
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 86, 0)
					size_t sent;
					const CURLcode send_result = curl_ws_send(easy, message.c_str(), message.length(), &sent, 0, CURLWS_TEXT);
					
					if (send_result == CURLE_OK)
					{
						ctx.out << "Sent " << sent << " bytes: " << message << "\n";
						
						// Try to receive a response
						char buffer[1024];
						size_t recv_bytes;
						const struct curl_ws_frame *meta;
						const CURLcode recv_result = curl_ws_recv(easy, buffer, sizeof(buffer) - 1, &recv_bytes, &meta);
						
						if (recv_result == CURLE_OK && recv_bytes > 0)
						{
							buffer[recv_bytes] = '\0';
							ctx.out << "Received " << recv_bytes << " bytes";
							if (meta)
							{
								ctx.out << " (frame type: ";
								if (meta->flags & CURLWS_TEXT) ctx.out << "TEXT";
								else if (meta->flags & CURLWS_BINARY) ctx.out << "BINARY";
								else if (meta->flags & CURLWS_PING) ctx.out << "PING";
								else if (meta->flags & CURLWS_PONG) ctx.out << "PONG";
								else if (meta->flags & CURLWS_CLOSE) ctx.out << "CLOSE";
								else ctx.out << "UNKNOWN";
								ctx.out << ")";
							}
							ctx.out << ": " << buffer << "\n";
						}
						else if (recv_result != CURLE_AGAIN)
						{
							ctx.err << "\e[91mws: receive failed: " << curl_easy_strerror(recv_result) << "\e[39m\n";
						}
					}
					else
					{
						ctx.err << "\e[91mws: send failed: " << curl_easy_strerror(send_result) << "\e[39m\n";
					}
#else
					ctx.out << "Note: Full WebSocket API not available in this curl version.\n";
					ctx.out << "Connection established but message sending requires curl >= 7.86.0.\n";
					ctx.out << "For full functionality, use the Lua 'websocket()' API for basic operation.\n";
#endif
				}
				else
				{
					ctx.out << "Connected to " << ctx.args[1] << ".\n";
					ctx.out << "Use 'ws <url> <message>' to send data (if supported).\n";
					ctx.out << "For full WebSocket functionality, use the Lua websocket() API.\n";
				}
			}
			done = true;
		});

	while (!done && pmMainLoop())
	{
		threadYield();
	}
#else
	const auto rc = curl_easy_perform(easy);
	if (rc != CURLE_OK)
	{
		ctx.err << "\e[91mws: " << curl_easy_strerror(rc) << ": " << curl_errbuf << "\e[39m\n";
	}
	else
	{
		ctx.out << "WebSocket connection established to " << ctx.args[1] << "\n";
		
		// If a message was provided, try to send it
		if (ctx.args.size() > 2)
		{
			const auto& message = ctx.args[2];
			
#if defined(CURL_AT_LEAST_VERSION) && CURL_AT_LEAST_VERSION(7, 86, 0)
			size_t sent;
			const CURLcode send_result = curl_ws_send(easy, message.c_str(), message.length(), &sent, 0, CURLWS_TEXT);
			
			if (send_result == CURLE_OK)
			{
				ctx.out << "Sent " << sent << " bytes: " << message << "\n";
				
				// Try to receive a response
				char buffer[1024];
				size_t recv_bytes;
				const struct curl_ws_frame *meta;
				const CURLcode recv_result = curl_ws_recv(easy, buffer, sizeof(buffer) - 1, &recv_bytes, &meta);
				
				if (recv_result == CURLE_OK && recv_bytes > 0)
				{
					buffer[recv_bytes] = '\0';
					ctx.out << "Received " << recv_bytes << " bytes";
					if (meta)
					{
						ctx.out << " (frame type: ";
						if (meta->flags & CURLWS_TEXT) ctx.out << "TEXT";
						else if (meta->flags & CURLWS_BINARY) ctx.out << "BINARY";
						else if (meta->flags & CURLWS_PING) ctx.out << "PING";
						else if (meta->flags & CURLWS_PONG) ctx.out << "PONG";
						else if (meta->flags & CURLWS_CLOSE) ctx.out << "CLOSE";
						else ctx.out << "UNKNOWN";
						ctx.out << ")";
					}
					ctx.out << ": " << buffer << "\n";
				}
				else if (recv_result != CURLE_AGAIN)
				{
					ctx.err << "\e[91mws: receive failed: " << curl_easy_strerror(recv_result) << "\e[39m\n";
				}
			}
			else
			{
				ctx.err << "\e[91mws: send failed: " << curl_easy_strerror(send_result) << "\e[39m\n";
			}
#else
			ctx.out << "Note: Full WebSocket API not available in this curl version.\n";
			ctx.out << "Connection established but message sending requires curl >= 7.86.0.\n";
#endif
		}
		else
		{
			ctx.out << "Connected to " << ctx.args[1] << ".\n";
			ctx.out << "Use 'ws <url> <message>' to send data (if supported).\n";
		}
	}
	
	curl_easy_cleanup(easy);
#endif
}