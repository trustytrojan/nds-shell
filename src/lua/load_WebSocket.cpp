#include "WebSocket.hpp"
#include "my_state.hpp"

void my_state::load_WebSocket()
{
	// clang-format off
	new_usertype<WebSocket>("WebSocket",
		"new", sol::constructors<WebSocket(std::string_view)>(),
		"connectTimeoutSeconds", sol::writeonly_property(&WebSocket::setConnectTimeout),
		"caFile", sol::writeonly_property(&WebSocket::setCaFile),
		"connect", &WebSocket::connect,
		"poll", &WebSocket::poll,
		"send", static_cast<CURLcode (WebSocket::*)(const std::string_view &)>(&WebSocket::send),
		"close", &WebSocket::close,
		"on_open", sol::writeonly_property(&WebSocket::on_open),
		"on_message", sol::writeonly_property(&WebSocket::on_message),
		"on_error", sol::writeonly_property(&WebSocket::on_error),
		"on_close", sol::writeonly_property(&WebSocket::on_close)
	);
	// clang-format on
}
