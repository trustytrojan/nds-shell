#include "my_state.hpp"

#include "CurlMulti.hpp"

#include <curl/curl.h>

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);

void my_state::load_websocket()
{
	set("websocket",
		[&](const sol::string_view &url,
			const sol::table &opts,
			sol::function on_connect) -> std::pair<sol::object, sol::object>
		{
			if (!on_connect.valid())
				return {sol::nil, sol::make_object(*this, "on_connect callback is not a valid function")};

			const auto easy = curl_easy_init();
			if (!easy)
				return {sol::nil, sol::make_object(*this, "curl_easy_init failed")};

			// Error buffer that needs to live until the callback is fired
			auto error_buffer = std::make_shared<std::array<char, CURL_ERROR_SIZE>>();
			(*error_buffer)[0] = '\0';

			// Most important! socket() fails otherwise
			curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

			// Set up WebSocket connection - use CONNECT_ONLY mode
			curl_easy_setopt(easy, CURLOPT_URL, url.data());
			curl_easy_setopt(easy, CURLOPT_CONNECT_ONLY, 2L); // 2 for WebSocket if supported, 1 for basic connection

			// Set timeout from options or default
			const auto timeout = opts["timeout"].get_or(10);
			curl_easy_setopt(easy, CURLOPT_TIMEOUT, timeout);
			curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);

			// Add headers if provided
			curl_slist *headers = nullptr;
			if (const auto headers_tbl = opts["headers"]; headers_tbl.is<sol::table>())
			{
				for (const auto &[k, v] : headers_tbl.get<sol::table>())
				{
					const auto header = k.as<std::string>() + ": " + v.as<std::string_view>();
					headers = curl_slist_append(headers, header.c_str());
				}
				curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
			}

			// CA cert for WSS
			if (Shell::fsInitialized())
				curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
			else if (url.find("wss://") == 0)
				ctx.err << "\e[91mwebsocket: wss unavailable: fs not initialized\e[39m\n";

			// Get curl errors
			curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer->data());

			// Create simplified WebSocket object
			auto ws_obj = create_table();
			ws_obj["url"] = std::string(url);
			ws_obj["connected"] = false;
			
			auto invoke_callbacks = [this, on_connect, error_buffer, ws_obj, easy](CURLcode result, long response_code)
			{
				if (result != CURLE_OK)
				{
					const auto error_str = std::string{curl_easy_strerror(result)} + ": " + error_buffer->data();
					on_connect(sol::nil, error_str);
				}
				else
				{
					// Mark as connected
					ws_obj["connected"] = true;
					
					// Create a simple info object
					auto info = create_table();
					info["url"] = ws_obj["url"];
					info["status"] = "connected";
					info["note"] = "Basic WebSocket connection established. Full WebSocket protocol implementation requires newer curl version.";
					
					// Connection established
					on_connect(info, sol::nil);
				}
			};

#ifdef NDSH_THREADING
			CurlMulti::AddEasyHandle(
				easy,
				[invoke_callbacks = std::move(invoke_callbacks), headers](CURLcode result, long response_code)
				{
					// This lambda is now executing on the CurlMulti thread.
					invoke_callbacks(result, response_code);

					// Now we can clean up the headers.
					if (headers)
						curl_slist_free_all(headers);
				});
#else
			const auto rc = curl_easy_perform(easy);

			long response_code = 0;
			curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);

			invoke_callbacks(rc, response_code);

			// Cleanup
			if (headers)
				curl_slist_free_all(headers);
			curl_easy_cleanup(easy);
#endif

			// Success. Return WebSocket object.
			return {ws_obj, sol::nil};
		});
}