#pragma once

#include <CurlMulti.hpp>
#include <curl/curl.h>
#include <nds.h>
#include <string_view>

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);

class WebSocket
{
	CURL *const easy;
	char curl_errbuf[CURL_ERROR_SIZE]{};
	bool _open{};

public:
	WebSocket(const char *url)
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

	WebSocket(const std::string_view &s)
		: WebSocket{s.data()}
	{
	}

	~WebSocket()
	{
		CurlMulti::RemoveEasyHandle(easy);
		curl_easy_cleanup(easy);
	}

	std::string_view getErrbuf() { return {curl_errbuf, CURL_ERROR_SIZE}; }

	void setConnectTimeout(int seconds)
	{
		curl_easy_setopt(easy, CURLOPT_TIMEOUT, seconds);
		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, seconds);
	}

	void setCaFile(const char *filename) { curl_easy_setopt(easy, CURLOPT_CAINFO, filename); }
	void setCaFile(const std::string_view &s) { setCaFile(s.data()); }

	CURLcode connect()
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

				if (respcode != 101) // Switching Protocols
					// we NEED to switch protocols! error if this didn't happen!
					rc = CURLE_HTTP_RETURNED_ERROR;

				done = true;
			});

		while (!done && pmMainLoop())
			threadYield();
#else
		if (const auto rc = curl_easy_perform(easy); rc != CURLE_OK)
			return rc;
#endif

		if (rc == CURLE_OK)
			_open = true;

		return rc;
	}

	// Direct C function wrapper (sending text only)
	CURLcode send(const char *buf, size_t len, size_t *sent)
	{
		return curl_ws_send(easy, buf, len, sent, 0, CURLWS_TEXT);
	}

	// Send the entire string s, calling curl_ws_send() several times if needed
	CURLcode send(const std::string_view &s)
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

	// Direct C function wrapper
	CURLcode recv(char *buf, size_t len, size_t *rlen, const curl_ws_frame **meta)
	{
		return curl_ws_recv(easy, buf, len, rlen, meta);
	}

	// Receives a WebSocket fragment (part of a frame), **appending** the data into `msgbuf`.
	// Returns: the CURLcode, and whether the received fragment was the final one.
	// *Closes the websocket* if we receive the close flag from the server.
	std::tuple<CURLcode, bool> recvFragment(std::vector<char> &msgbuf)
	{
		const curl_ws_frame *meta{};
		char buffer[1024];
		size_t rlen;

		switch (const auto rc = recv(buffer, sizeof(buffer), &rlen, &meta))
		{
		case CURLE_AGAIN:
			return {rc, true};
		case CURLE_OK:
			if (meta->flags & CURLWS_CLOSE)
			{
				_open = false;
				return {rc, true};
			}
			msgbuf.insert(msgbuf.end(), buffer, buffer + rlen);
			return {rc, !(meta->flags & CURLWS_CONT)};
		default:
			return {rc, true};
		}
	}

	CURLcode close()
	{
		_open = false;
		size_t sent;
		return curl_ws_send(easy, "", 0, &sent, 0, CURLWS_CLOSE);
	}

	bool open() const { return _open; }
};
