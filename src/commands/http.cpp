#include "Commands.hpp"
#include "Shell.hpp"

#include <algorithm>
#include <curl/curl.h>

#include <cstring>
#include <iostream>

// Debug callback to print libcurl logs
static int CurlDebugCallback(
	CURL *, curl_infotype type, char *data, size_t size, void *)
{
	switch (type)
	{
	case CURLINFO_TEXT:
	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
	case CURLINFO_DATA_IN:
	case CURLINFO_DATA_OUT:
		std::cerr << "\e[40m";
		std::cerr.write(data, size);
		std::cerr << "\e[39m";
		break;
	default:
		break;
	}
	return 0;
}

void Commands::http()
{
	if (Shell::args.size() != 3)
	{
		std::cerr << "usage: http <method> <url>\n";
		return;
	}

	// grab arguments
	std::string method(Shell::args[1].length(), '\0');
	const auto &url = Shell::args[2];

	// ensure uppercase method
	std::ranges::transform(Shell::args[1], method.begin(), toupper);

	const auto curl = curl_easy_init();
	if (!curl)
	{
		std::cerr << "\e[41mhttp: curl_easy_init failed\e[39m\n";
		return;
	}

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// write http response to stdout
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	curl_easy_setopt(
		curl, CURLOPT_OPENSOCKETFUNCTION,
		[](auto, auto, auto address)
		{ return socket(address->family, address->socktype, 0); }
	);

	// get curl errors
	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);

	// debug output
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, CurlDebugCallback);

	if (const auto res = curl_easy_perform(curl); res != CURLE_OK)
	{
		std::cerr << "\e[41mhttp: curl: " << curl_easy_strerror(res) << ": "
				  << curl_errbuf << "\e[39m\n";
	}

	curl_easy_cleanup(curl);
}
