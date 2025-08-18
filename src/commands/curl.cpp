#include "Commands.hpp"
#include "CurlMulti.hpp"

#include <curl/curl.h>

#ifdef CURL_DEBUG
static int curl_debug(CURL *, curl_infotype type, char *const data, const size_t size, void *userp)
{
	if (type != CURLINFO_TEXT)
		// we only want debug messages, not req/resp data
		return 0;
	auto &ostr = *reinterpret_cast<std::ostream *>(userp);
	ostr << "\e[90m";
	ostr.write(data, size);
	ostr << "\e[39m";
	return 0;
}
#endif

curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr)
{
	return socket(addr->family, addr->socktype, 0);
}

size_t curl_write(char *const buffer, const size_t size, const size_t nitems, void *userp)
{
	const auto bytes = size * nitems;
	// we can safely pass the whole buffer because of https://github.com/devkitPro/libnds/pull/72
	// (not merged yet, using my fork)
	reinterpret_cast<std::ostream *>(userp)->write(buffer, bytes);
	return bytes;
}

void Commands::curl(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: curl <url>\n";
		return;
	}

	const auto easy = curl_easy_init();
	if (!easy)
	{
		ctx.err << "\e[91mhttp: curl_easy_init failed\e[39m\n";
		return;
	}

	curl_easy_setopt(easy, CURLOPT_URL, ctx.args[1].c_str());

	// set a timeout so we aren't stuck connecting/reading
	const auto timeout = atoi(ctx.GetEnv("CURL_TIMEOUT", "5").c_str());
	curl_easy_setopt(easy, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, timeout);

	// CA cert bundle file, set the filename using env
	// copy one from a linux system and it works flawlessly
	if (Shell::fsInitialized())
		curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
	else
		ctx.err << "\e[91mcurl: https unavailable: fs not initialized\e[39m\n";

	// write http response to ctx.out
	curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curl_write);
	curl_easy_setopt(easy, CURLOPT_WRITEDATA, &ctx.out);

	// we need a custom opensocket callback because of
	// https://github.com/devkitPro/dswifi/blob/f61bbc661dc7087fc5b354cd5ec9a878636d4cbf/source/sgIP/sgIP_sockets.c#L98
	// note to self... DO NOT USE C++ LAMBDAS
	curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

	// get curl errors
	char curl_errbuf[CURL_ERROR_SIZE]{};
	curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, curl_errbuf);

#ifdef CURL_DEBUG
	// debug output
	if (ctx.env.contains("CURL_DEBUG"))
	{
		curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(easy, CURLOPT_DEBUGFUNCTION, curl_debug);
		curl_easy_setopt(easy, CURLOPT_DEBUGDATA, &ctx.err);
	}
#endif

#ifdef NDSH_THREADING
	// use a multi to make the easy nonblocking!
	// but since this is the `curl` command, we want to block until it's done.
	// we can use a simple flag and the callback system to achieve this.
	bool done = false;
	CurlMulti::AddEasyHandle(
		easy,
		[&](CURLcode result, long)
		{
			if (result != CURLE_OK)
				ctx.err << "\e[91mcurl: " << curl_easy_strerror(result) << ": " << curl_errbuf << "\e[39m\n";
			done = true;
		});

	while (!done && pmMainLoop())
		threadYield();
#else
	if (const auto rc = curl_easy_perform(easy); rc != CURLE_OK)
		ctx.err << "\e[91mcurl: " << curl_easy_strerror(rc) << ": " << curl_errbuf << "\e[39m\n";
#endif

	// curl_easy_cleanup(easy); // CurlMulti owns the handle now
}
