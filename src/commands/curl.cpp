#include "Commands.hpp"
#include "Shell.hpp"

#include <algorithm>
#include <curl/curl.h>

#include <cstring>
#include <iostream>

static int
curl_debug(CURL *, curl_infotype type, char *data, size_t size, void *)
{
	switch (type)
	{
	case CURLINFO_TEXT:
	case CURLINFO_HEADER_IN:
	case CURLINFO_HEADER_OUT:
	case CURLINFO_DATA_IN:
	case CURLINFO_DATA_OUT:
	case CURLINFO_SSL_DATA_IN:
	case CURLINFO_SSL_DATA_OUT:
		*Shell::err << "\e[40m";
		Shell::err->write(data, size);
		*Shell::err << "\e[39m";
	default:
		break;
	}
	return 0;
}

static curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *addr)
{
	return socket(addr->family, addr->socktype, 0);
}

size_t curl_write(char *buffer, size_t size, size_t nitems, void *)
{
	Shell::out->write(buffer, size * nitems);
	return size * nitems;
}

void Commands::curl()
{
	if (Shell::args.size() < 2)
	{
		*Shell::err << "usage: curl [method] <url>\n";
		return;
	}

	// grab arguments
	std::string method = "GET";
	if (Shell::args.size() >= 3)
	{ // method is optional
		method.resize(Shell::args[1].length());
		std::ranges::transform(Shell::args[1], method.begin(), toupper);
	}

	const auto &url = Shell::args.back();

	const auto curl = curl_easy_init();
	if (!curl)
	{
		*Shell::err << "\e[41mhttp: curl_easy_init failed\e[39m\n";
		return;
	}

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	// copy your OS's CA cert bundle onto the SD card
	// (rename this string if necessary)
	curl_easy_setopt(curl, CURLOPT_CAINFO, "tls-ca-bundle.pem");

	// by default, http response is written to stdout
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	// note to self... DO NOT USE C++ LAMBDAS
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	// get curl errors
	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);

	// debug output
	// TODO: make this optional
	// curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
	// curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug);

	if (const auto res = curl_easy_perform(curl); res != CURLE_OK)
	{
		*Shell::err << "\e[41mhttp: curl: " << curl_easy_strerror(res) << ": "
					<< curl_errbuf << "\e[39m\n";
	}

	curl_easy_cleanup(curl);
}
