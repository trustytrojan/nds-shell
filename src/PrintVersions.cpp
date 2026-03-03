#include <iostream>

#ifdef NDSH_CURL
	#include <curl/curl.h>
#endif

#ifdef NDSH_LIBSSH2
	#include "libssh2.h"
#endif

#ifdef NDSH_SOL2
	#include "lua.h"
	#include <sol/sol.hpp>
#endif

void PrintVersions()
{
#ifdef NDSH_CURL
	std::cout << curl_version() << ' ';
	const auto protocols = curl_version_info(CURLVERSION_NOW)->protocols;
	for (auto p = protocols; *p; ++p)
		std::cout << *p << ' ';
	std::cout << '\n';
#endif

#ifdef NDSH_LIBSSH2
	std::cout << "libssh2 " LIBSSH2_VERSION << '\n';
#endif

#ifdef NDSH_SOL2
	std::cout << LUA_RELEASE << " sol2 " << SOL_VERSION_STRING << '\n';
#endif

#ifdef NDSH_LIBTELNET
	std::cout << "libtelnet\n";
#endif

	std::cout << '\n';
}
