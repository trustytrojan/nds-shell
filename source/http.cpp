#include "everything.hpp"
#include "NetParse.hpp"

void http(const Args &args)
{
	if (args.size() != 3)
	{
		std::cerr << "args: <method> <url>\n<url>: `http://` MUST be omitted\n\e[31mnote: https is unsupported\e[39m\n";
		return;
	}

	// convert to uppercase
	auto method = args[1];
	std::transform(method.cbegin(), method.cend(), method.begin(), toupper);

	// check against supported methods
	static const std::unordered_set<std::string> httpMethods{"GET", "POST", "PUT", "DELETE"};
	if (std::find(httpMethods.cbegin(), httpMethods.cend(), method) == httpMethods.cend())
	{
		std::cerr << "\e[41minvalid http method\e[39m\n\nvalid http methods:\n";
		for (const auto &s : httpMethods)
			std::cerr << '\t' << s << '\n';
		std::cerr << '\n';
		return;
	}

	// parse out address and path from url
	auto url = args[2];

	// take out http:// from url if its there
	if (strncmp(url.c_str(), "http://", 7) == 0) // faster than taking a substring for a comparison!
	{
		url = url.substr(7);
	}

	std::string address, path;

	const auto slashIndex = url.find('/');
	if (slashIndex == std::string::npos)
	{
		address = url;
		path = "/";
	}
	else
	{
		address = url.substr(0, slashIndex);
		path = url.substr(slashIndex);
	}

	sockaddr_in sain;
	if (!NetParse::parseAddress(address, 80, sain))
	{
		NetParse::printError("http");
		return;
	}

	const auto sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
		perror("socket");
		return;
	}

	if (connect(sock, (sockaddr *)&sain, sizeof(sockaddr_in)) == -1)
	{
		perror("connect");
		return;
	}

	std::stringstream ss;
	ss << method << ' ' << path << " HTTP/1.1\r\nHost: " << address << "\r\nUser-Agent: Nintendo DS\r\n\r\n";

	const auto request = ss.str();

	if (send(sock, request.c_str(), request.size(), 0) == -1)
	{
		perror("send");
		return;
	}

	char responseBuffer[1024];
	int bytesReceived;
	switch (bytesReceived = recv(sock, responseBuffer, sizeof(responseBuffer), 0))
	{
	case -1:
		perror("recv");
		return;
	case 0:
		std::cerr << "server closed connection\n";
		return;
	}

	/**
	 * NOTE: closing the socket breaks stdout!
	 */

	// if (close(sock) == -1)
	// {
	// 	perror("close");
	// 	return;
	// }

	// null-terminate the string
	responseBuffer[bytesReceived + 1] = 0;

	// maybe omit the added newline later
	std::cout << responseBuffer << '\n';
}
