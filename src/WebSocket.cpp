#include "WebSocket.hpp"
#include <arpa/inet.h>

WebSocket::WebSocket(const char *url)
	: easy{curl_easy_init()}
{
	curl_easy_setopt(easy, CURLOPT_URL, url);

	// REQUIRED to actually have a bidirectional WebSocket that we call curl_ws_send/recv on
	curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 2L);

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	// note to self... DO NOT USE C++ LAMBDAS
	curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_errbuf);
}

WebSocket::WebSocket(const std::string_view &s)
	: WebSocket{s.data()}
{
}

WebSocket::~WebSocket()
{
	close();
	CurlMulti::RemoveEasyHandle(easy);
	curl_easy_cleanup(easy);
}

CURLcode WebSocket::connect()
{
	// This entire block of code below, regardless of NDSH_THREADING, simply connects to the WebSocket server.
	// **Only afterwards** can we begin using curl_ws_send and curl_ws_recv.
#ifdef NDSH_THREADING
	bool done{};
	CURLcode rc = CURLE_OK;

	CurlMulti::AddEasyHandle(
		easy,
		[&](CURLcode result, long respcode)
		{
			rc = result;

			if (rc != CURLE_OK && _on_error)
				_on_error(rc, curl_errbuf);

			if (respcode != 101 && _on_error)
				_on_error(CURLE_HTTP_RETURNED_ERROR, "did not receive HTTP 101 Switching Protocols!");

			done = true;
		});

	while (!done && pmMainLoop())
		threadYield();
#else
	if (const auto rc = curl_easy_perform(easy); rc != CURLE_OK)
		return rc;
#endif

	if (rc == CURLE_OK && _on_open)
		_on_open();

	return rc;
}

CURLcode WebSocket::send(const char *buf, size_t len, size_t *sent)
{
	return curl_ws_send(easy, buf, len, sent, 0, CURLWS_TEXT);
}

CURLcode WebSocket::send(const std::string_view &s)
{
	CURLcode rc;
	size_t sent;
	auto buf = s.data();
	auto blen = s.size();

	while (blen && pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif
		switch (rc = send(buf, blen, &sent))
		{
		case CURLE_AGAIN:
			break;
		case CURLE_OK:
			buf += sent;
			blen -= sent;
			break;
		default:
			return rc;
		}
	}

	return rc;
}

CURLcode WebSocket::recv(char *buf, size_t len, size_t *rlen, const curl_ws_frame **meta)
{
	return curl_ws_recv(easy, buf, len, rlen, meta);
}

void WebSocket::poll()
{
	const curl_ws_frame *meta{};
	char buffer[1024];
	size_t rlen;

	switch (const auto rc = recv(buffer, sizeof(buffer), &rlen, &meta))
	{
	case CURLE_AGAIN:
		break;

	case CURLE_OK:
		if (meta->flags & CURLWS_CLOSE)
		{
			if (_on_close)
			{
				if (rlen >= 2)
				{
					// 2-byte close code
					u16 code;
					memcpy(&code, buffer, 2);
					code = ntohs(code);

					// clear whatever data we were previously accumulating,
					// and read in the close reason instead for on_close.
					_msgbuf.clear();
					if (rlen > 2)
					{
						const auto start = buffer + 2;
						_msgbuf.insert(_msgbuf.end(), start, start + std::min(rlen, 123u));
					}

					_on_close(code, _msgbuf);
				}
				else
				{
					// treat it like a normal close
					_on_close(1000, "");
				}
			}

			// send a close frame back
			close();
			return;
		}

		_msgbuf.insert(_msgbuf.end(), buffer, buffer + rlen);

		// Final fragment of frame? Fire on_message!
		if (!(meta->flags & CURLWS_CONT) && _on_message)
		{
			_on_message(_msgbuf);
			_msgbuf.clear();
		}

		break;

	default:
		if (_on_error)
			_on_error(rc, curl_errbuf);
		break;
	}
}

CURLcode WebSocket::close()
{
	size_t sent;
	return curl_ws_send(easy, "", 0, &sent, 0, CURLWS_CLOSE);
}
