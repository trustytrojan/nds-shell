#include "Commands.hpp"
#include "Shell.hpp"

#include <curl/curl.h>

#include <cstring>
#include <iostream>

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	const auto totalSize = size * nmemb;
	std::cout.write(static_cast<const char *>(contents), totalSize);
	return totalSize;
}

static curl_socket_t OpenSocketCallback(
	void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
	return socket(address->family, address->socktype, 0);
}

void Commands::http()
{
	if (Shell::args.size() != 3)
	{
		std::cerr << "usage: http <method> <url>\n";
		return;
	}

	const auto &method = Shell::args[1];
	const auto &url = Shell::args[2];

	char errorBuffer[CURL_ERROR_SIZE]{}; // Buffer to store error messages

	// Initialize libcurl
	const auto curl = curl_easy_init();
	if (!curl)
	{
		std::cerr << "Failed to initialize libcurl\n";
		return;
	}

	// Set libcurl options
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, OpenSocketCallback);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
	// curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "http");

	// Perform the request
	if (const auto res = curl_easy_perform(curl); res != CURLE_OK)
	{
		std::cerr << "\e[41mhttp: curl: " << curl_easy_strerror(res) << ": "
				  << errorBuffer << "\e[39m\n";
	}

	// Cleanup
	curl_easy_cleanup(curl);
}
