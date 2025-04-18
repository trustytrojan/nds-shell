#include "Commands.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <cstring>
#include <curl/curl.h>
#include <iostream>
#include <unistd.h>

extern "C"
{
	// basename doesn't exist in libnds? libcurl depends on it,
	// so here's a crude implementation.
	char *basename(const char *path)
	{
		if (*path == '\0')
			// Return current directory for empty paths
			return const_cast<char *>(".");

		const auto base = strrchr(path, '/');
		return base ? (base + 1) : const_cast<char *>(path);
	}
}

static size_t
WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	const auto totalSize = size * nmemb;
	std::cout.write(static_cast<const char *>(contents), totalSize);
	return totalSize;
}

// Custom opensocket callback
static curl_socket_t OpenSocketCallback(
	void *clientp, curlsocktype purpose, struct curl_sockaddr *address)
{
	// Extract the pre-connected socket from clientp
	return *(curl_socket_t *)clientp;
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

	// Parse the URL to extract the host and path
	std::string host, path;
	int port = 80; // Default HTTP port
	if (url.find("http://") == 0)
	{
		const auto hostStart = url.find("http://") + 7;
		const auto pathStart = url.find('/', hostStart);
		host = url.substr(hostStart, pathStart - hostStart);
		path = url.substr(pathStart);
	}
	else
	{
		std::cerr << "Only HTTP URLs are supported\n";
		return;
	}

	// Use NetUtils::ParseAddress to resolve the host and port
	sockaddr_in serverAddr{};
	if (!NetUtils::ParseAddress(host.c_str(), port, serverAddr))
	{
		NetUtils::PrintError("Failed to parse address");
		return;
	}

	// Create and connect the socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		std::cerr << "Failed to create socket\n";
		return;
	}

	if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		std::cerr << "Failed to connect to host: " << host << "\n";
		closesocket(sockfd);
		return;
	}

	// Initialize libcurl
	const auto curl = curl_easy_init();
	if (!curl)
	{
		std::cerr << "Failed to initialize libcurl\n";
		closesocket(sockfd);
		return;
	}

	// Set libcurl options
	curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, OpenSocketCallback);
	curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, &sockfd);

	// Perform the request
	if (const auto res = curl_easy_perform(curl); res != CURLE_OK)
	{
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res)
				  << "\n";
	}

	// Cleanup
	curl_easy_cleanup(curl);
	closesocket(sockfd);
}
