#pragma once

#include "NetUtils.hpp"

#include <cerrno>
#include <cstring>
#include <expected>
#include <fcntl.h>
#include <netinet/in.h>
#include <string_view>
#include <sys/socket.h>
#include <unistd.h>

#ifndef __BLOCKSDS__
	// only dkp defines it separately in this header
	// blocksds though... defines it to -1? can't let that happen
	#include <sys/ioctl.h>
#endif

struct TcpSocket
{
	const int fd{socket(AF_INET, SOCK_STREAM, 0)};
	constexpr ~TcpSocket() { closesocket(fd); }

	// direct system call wrappers
	constexpr int connect(sockaddr_in &sain) const { return ::connect(fd, (sockaddr *)&sain, sizeof(sockaddr_in)); }
	constexpr int recv(void *data, size_t recvlength) const { return ::recv(fd, data, recvlength, 0); }
	constexpr int send(const void *data, size_t sendlength) const { return ::send(fd, data, sendlength, 0); }

	constexpr std::expected<void, const char *> connect(const std::string_view &hostport, const int defaultPort = -1)
	{
		sockaddr_in sain;
		if (const auto rc = NetUtils::ParseAddress(sain, hostport.data(), defaultPort); rc != NetUtils::Error::NO_ERROR)
			return std::unexpected{NetUtils::StrError(rc)};
		if (connect(sain) == -1)
			return std::unexpected{strerror(errno)};
		return {};
	}

	constexpr int send(const std::string_view &s) const { return send(s.data(), s.size()); }

	constexpr std::expected<void, const char *> setNonblocking(bool yes) const
	{
		// blocksds: we will rely on ioctl() because it is easier to use than fcntl()
		if (::ioctl(fd, FIONBIO, &yes) == -1)
			return std::unexpected{strerror(errno)};
		return {};
	}
};
