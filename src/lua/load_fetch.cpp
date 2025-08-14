#include "my_state.hpp"

#include "CurlMulti.hpp"

#include <curl/curl.h>

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);
// userp must be a std::ostream *
size_t curl_write(char *const buffer, const size_t size, const size_t nitems, void *userp);

void my_state::load_fetch()
{
	set("fetch",
		[&](const sol::string_view &url,
			const sol::table &opts,
			sol::function on_complete) -> std::pair<sol::object, sol::object>
		{
			if (!on_complete.valid())
				return {sol::nil, sol::make_object(*this, "on_complete callback is not a valid function")};

			const auto easy = curl_easy_init();
			if (!easy)
				return {sol::nil, sol::make_object(*this, "curl_easy_init failed")};

			// Data that needs to live until the callback is fired.
			// For the sync case, shared_ptr is slight overkill but allows code reuse.
			auto response_body = std::make_shared<std::ostringstream>();
			auto error_buffer = std::make_shared<std::array<char, CURL_ERROR_SIZE>>();
			(*error_buffer)[0] = '\0';

			// most important! socket() fails otherwise
			curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

			// url, method, body
			curl_easy_setopt(easy, CURLOPT_URL, url.data());
			if (const auto method = opts["method"])
				curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.get<sol::string_view>().data());
			if (const auto body = opts["body"])
				curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, body.get<sol::string_view>().data());

			// Add headers
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

			if (Shell::fsInitialized())
				curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
			else
				ctx.err << "\e[91mcurl: https unavailable: fs not initialized\e[39m\n";

			// capture output
			curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curl_write);
			curl_easy_setopt(easy, CURLOPT_WRITEDATA, response_body.get());

			// get curl errors
			curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer->data());

			auto invoke_callback = [on_complete, response_body, error_buffer](CURLcode result, long response_code)
			{
				if (result != CURLE_OK)
				{
					const auto error_str = std::string{curl_easy_strerror(result)} + ": " + error_buffer->data();
					on_complete(response_code, sol::nil, error_str);
				}
				else
				{
					on_complete(response_code, response_body->str(), sol::nil);
				}
			};

#ifdef NDSH_THREADING
			CurlMulti::AddEasyHandle(
				easy,
				[invoke_callback = std::move(invoke_callback), headers](CURLcode result, long response_code)
				{
					// This lambda is now executing on the CurlMulti thread.
					invoke_callback(result, response_code);

					// Now we can clean up the headers.
					if (headers)
						curl_slist_free_all(headers);
				});
#else
			const auto rc = curl_easy_perform(easy);

			long response_code = 0;
			curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);

			invoke_callback(rc, response_code);

			// Cleanup
			if (headers)
				curl_slist_free_all(headers);
			curl_easy_cleanup(easy);
#endif

			// Success. Return true.
			return {sol::make_object(*this, true), sol::nil};
		});
}
