#include "Commands.hpp"
#include "Shell.hpp"

#include <algorithm>
#include <curl/curl.h>
#include <iostream>

static int
curl_debug(CURL *, curl_infotype, char *const data, const size_t size, void *)
{
	*Shell::err << "\e[40m";
	Shell::err->write(data, size);
	*Shell::err << "\e[39m";
	return 0;
}

static curl_socket_t
curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr)
{
	return socket(addr->family, addr->socktype, 0);
}

size_t
curl_write(char *const buffer, const size_t size, const size_t nitems, void *)
{
	// manually put chars because putting a char* can cause a memory error here:
	// https://github.com/devkitPro/libnds/blob/6194b32d8f94e2ebc8078e64bf213ffc13ba1985/source/arm9/console.c#L223
	// looks like they didn't check for null chars in the loop condition
	const auto bytes = size * nitems;
	const char *const endp = buffer + bytes;
	for (const char *p = buffer; *p && p < endp; ++p)
		*Shell::out << *p;
	return bytes;
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

	// set a timeout so we aren't stuck connecting/reading
	const auto timeout = atoi(Shell::GetEnv("CURL_TIMEOUT", "10").c_str());
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);

	// CA cert bundle file, set the filename using env
	// copy one from a linux system and it works flawlessly
	curl_easy_setopt(
		curl,
		CURLOPT_CAINFO,
		Shell::GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());

	// write http response to *Shell::out
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	// note to self... DO NOT USE C++ LAMBDAS
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	// get curl errors
	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_errbuf);

	// debug output
	if (Shell::HasEnv("CURL_DEBUG"))
	{
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug);
	}

	if (const auto res = curl_easy_perform(curl); res != CURLE_OK)
	{
		*Shell::err << "\e[41mhttp: curl: " << curl_easy_strerror(res) << ": "
					<< curl_errbuf << "\e[39m\n";
	}

	curl_easy_cleanup(curl);
}
