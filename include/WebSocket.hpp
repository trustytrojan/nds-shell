#pragma once

#include <CurlMulti.hpp>
#include <curl/curl.h>
#include <functional>
#include <nds.h>
#include <string>
#include <string_view>

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);

class WebSocket
{
public:
	using OpenCb = std::function<void()>;
	using MessageCb = std::function<void(const std::string &)>;
	using ErrorCb = std::function<void(CURLcode, std::string_view)>;
	using CloseCb = std::function<void(u16, const std::string &)>;

private:
	CURL *const easy;
	char curl_errbuf[CURL_ERROR_SIZE]{};
	std::string _msgbuf;
	OpenCb _on_open;
	MessageCb _on_message;
	ErrorCb _on_error;
	CloseCb _on_close;

public:
	WebSocket(const char *url);
	WebSocket(const std::string_view &s);
	~WebSocket();

	void setConnectTimeout(int seconds)
	{
		curl_easy_setopt(easy, CURLOPT_TIMEOUT, seconds);
		curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, seconds);
	}
	void setCaFile(const std::string_view &s) { curl_easy_setopt(easy, CURLOPT_CAINFO, s.data()); }

	void on_open(const OpenCb &cb) { _on_open = cb; }
	void on_message(const MessageCb &cb) { _on_message = cb; }
	void on_error(const ErrorCb &cb) { _on_error = cb; }
	void on_close(const CloseCb &cb) { _on_close = cb; }

	// Connects the WebSocket to the endpoint URL.
	// Blocks until connected, yielding the current thread.
	CURLcode connect();

	// Send the entire string s, calling curl_ws_send() several times if needed
	CURLcode send(const std::string_view &s);

	// Receive data using curl_ws_recv() and fire events as needed.
	void poll();

	// For now, just sends an empty close frame, as I don't have a need to specify to servers
	// why I'm closing the WebSocket. It doesn't benefit the client-side at all.
	CURLcode close();

private:
	// Direct C function wrapper (sending text only)
	CURLcode send(const char *buf, size_t len, size_t *sent);

	// Direct C function wrapper
	CURLcode recv(char *buf, size_t len, size_t *rlen, const curl_ws_frame **meta);
};
