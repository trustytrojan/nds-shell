#include "Commands.hpp"
#include <libssh2.h>
#include <nds.h>

// For networking
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

void Commands::ssh(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: ssh <address>" << std::endl;
		return;
	}

	std::string address_arg = ctx.args[1];
	std::string user;
	std::string address;

	size_t at_pos = address_arg.find('@');
	if (at_pos != std::string::npos)
	{
		user = address_arg.substr(0, at_pos);
		address = address_arg.substr(at_pos + 1);
	}
	else
	{
		address = address_arg;
	}

	int sock = -1;
	LIBSSH2_SESSION *session = nullptr;
	LIBSSH2_CHANNEL *channel = nullptr;
	char *err_msg = nullptr;

	do
	{
		if (libssh2_init(0))
		{
			ctx.err << "libssh2 initialization failed" << std::endl;
			break;
		}

		// Create socket and connect
		struct hostent *host = gethostbyname(address.c_str());
		if (!host)
		{
			ctx.err << "Could not resolve host: " << address << std::endl;
			break;
		}

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
		{
			ctx.err << "Failed to create socket: " << strerror(errno) << std::endl;
			break;
		}

		struct sockaddr_in sin;
		sin.sin_family = AF_INET;
		sin.sin_port = htons(22);
		sin.sin_addr = *(struct in_addr *)host->h_addr_list[0];

		if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) != 0)
		{
			ctx.err << "Failed to connect to " << address << ": " << strerror(errno) << std::endl;
			break;
		}

		// Create libssh2 session
		session = libssh2_session_init();
		if (!session)
		{
			ctx.err << "Failed to create libssh2 session" << std::endl;
			break;
		}

		// Handshake
		if (libssh2_session_handshake(session, sock))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "SSH handshake failed: " << err_msg << std::endl;
			break;
		}
		ctx.out << "Connected to " << address << std::endl;

		// Authentication
		// IMPORTANT: MbedTLS expects private keys to be in PEM format, NOT OPENSSH FORMAT!
		// This works: ssh-keygen -t rsa -b 2048 -m PEM -C "user@nds" -f /mnt/sdcard/.ssh/id_rsa
		if (const auto rc = libssh2_userauth_publickey_fromfile_ex(
				session, user.c_str(), user.size(), ".ssh/id_rsa.pub", ".ssh/id_rsa", NULL))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "ssh: pubkey auth failed: " << err_msg << " (" << rc << ")\n";

			// public key failed, check for password
			const char *userauthlist = libssh2_userauth_list(session, user.c_str(), user.length());
			if (strstr(userauthlist, "password") == NULL)
			{
				ctx.err << "ssh: passwd login not allowed" << std::endl;
				break;
			}

			std::string password;
			// a bit of a hack to get a password
			{
				ctx.out << "Password: ";
				std::string pass_buf;
				while (pmMainLoop())
				{
					threadYield();
					int key = keyboardUpdate();
					if (key > 0)
					{
						if (key == '\n' || key == '\r')
						{
							ctx.out << std::endl;
							break;
						}
						else if (key == '\b')
						{
							if (!pass_buf.empty())
							{
								pass_buf.pop_back();
								ctx.out << "\b \b";
							}
						}
						else
						{
							pass_buf += (char)key;
							ctx.out << '*';
						}
					}
				}
				password = pass_buf;
			}

			if (libssh2_userauth_password(session, user.c_str(), password.c_str()))
			{
				libssh2_session_last_error(session, &err_msg, NULL, 0);
				ctx.err << "ssh: Password authentication failed: " << err_msg << std::endl;
				break;
			}
		}
		ctx.out << "Authentication successful." << std::endl;

		// Open channel
		channel = libssh2_channel_open_session(session);
		if (!channel)
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to open channel: " << err_msg << std::endl;
			break;
		}

		// Request PTY
		if (libssh2_channel_request_pty(channel, "vanilla"))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to request PTY: " << err_msg << std::endl;
			break;
		}

		// Request shell
		if (libssh2_channel_shell(channel))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to request shell: " << err_msg << std::endl;
			break;
		}
		ctx.out << "Shell opened." << std::endl;

		libssh2_session_set_blocking(session, 0);
		libssh2_channel_set_blocking(channel, 0);

		int flags = 1;
		if (ioctl(sock, FIONBIO, &flags) == -1)
		{
			ctx.err << "ioctl: " << strerror(errno) << '\n';
			break;
		}

		while (!libssh2_channel_eof(channel) && pmMainLoop())
		{
			threadYield();

			char buffer[1024];
			ssize_t nbytes;

			// Read from channel
			nbytes = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (nbytes > 0)
			{
				// ctx.out.write(buffer, nbytes);
				for (char c : std::string_view{buffer, (size_t)nbytes})
					ctx.out << c; // prevent console from parsing escape sequences, let us see all the characters
			}
			else if (nbytes < 0 && nbytes != LIBSSH2_ERROR_EAGAIN)
			{
				libssh2_session_last_error(session, &err_msg, NULL, 0);
				ctx.err << "Error reading from channel: " << err_msg << std::endl;
				break;
			}

			if (ctx.shell.IsFocused())
			{
				// Check for keyboard input and send it
				int key = keyboardUpdate();
				if (key > 0)
				{
					char c = (char)key;
					ssize_t nwritten = libssh2_channel_write(channel, &c, 1);
					if (nwritten < 0 && nwritten != LIBSSH2_ERROR_EAGAIN)
					{
						libssh2_session_last_error(session, &err_msg, NULL, 0);
						ctx.err << "Error writing to channel: " << err_msg << std::endl;
						break;
					}
				}
			}
		}
	} while (0);

	if (channel)
	{
		libssh2_channel_close(channel);
		libssh2_channel_free(channel);
	}
	if (session)
	{
		libssh2_session_disconnect(session, "Normal Shutdown");
		libssh2_session_free(session);
	}
	if (sock >= 0)
		close(sock);
	libssh2_exit();
}
