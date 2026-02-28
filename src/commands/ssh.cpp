#include "Commands.hpp"
#include "NetUtils.hpp"
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
		ctx.err << "usage: ssh <address>\n";
		return;
	}

	const auto &address_arg = ctx.args[1];
	std::string user, address;

	const auto at_pos = address_arg.find('@');
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
			ctx.err << "libssh2 initialization failed\n";
			break;
		}

		// Create socket and connect
		sockaddr_in sin;
		if (const auto err = NetUtils::ParseAddress(sin, address, 22); err != NetUtils::Error::NO_ERROR)
		{
			ctx.err << "Could not parse address: " << NetUtils::StrError(err) << '\n';
			break;
		}

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0)
		{
			ctx.err << "Failed to create socket: " << strerror(errno) << '\n';
			break;
		}

		if (connect(sock, (struct sockaddr *)(&sin), sizeof(struct sockaddr_in)) != 0)
		{
			ctx.err << "Failed to connect to " << address << ": " << strerror(errno) << '\n';
			break;
		}

		// Create libssh2 session
		session = libssh2_session_init();
		if (!session)
		{
			ctx.err << "Failed to create libssh2 session\n";
			break;
		}

		// Handshake
		if (libssh2_session_handshake(session, sock))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "SSH handshake failed: " << err_msg << '\n';
			break;
		}
		ctx.out << "Connected to " << address << '\n';

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
				ctx.err << "ssh: passwd login not allowed\n";
				break;
			}

			std::string password;
			// a bit of a hack to get a password
			{
				ctx.out << "Password: ";
				std::string pass_buf;
				while (pmMainLoop())
				{
#ifdef NDSH_THREADING
					threadYield();
#else
					swiWaitForVBlank();
#endif
					int key = keyboardUpdate();
					if (key > 0)
					{
						if (key == '\n' || key == '\r')
						{
							ctx.out << '\n';
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
				ctx.err << "ssh: Password authentication failed: " << err_msg << '\n';
				break;
			}
		}
		ctx.out << "Authentication successful.\n";

		// Open channel
		channel = libssh2_channel_open_session(session);
		if (!channel)
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to open channel: " << err_msg << '\n';
			break;
		}

		// Request PTY
		// we need to be a "dumb" terminal, otherwise we get sent
		// a bunch of shell/session metadata we don't care about
		if (libssh2_channel_request_pty(channel, "dumb"))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to request PTY: " << err_msg << '\n';
			break;
		}

		// Request shell
		if (libssh2_channel_shell(channel))
		{
			libssh2_session_last_error(session, &err_msg, NULL, 0);
			ctx.err << "Failed to request shell: " << err_msg << '\n';
			break;
		}
		ctx.out << "Shell opened.\n";

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
#ifdef NDSH_THREADING
			threadYield();
#else
			swiWaitForVBlank();
#endif
			char buffer[1024];

			// Read from channel
			const auto nbytes = libssh2_channel_read(channel, buffer, sizeof(buffer));
			if (nbytes > 0)
			{
				for (const auto c : std::string_view{buffer, static_cast<size_t>(nbytes)})
					if (c == '\b')
						ctx.out << "\e[D"; // move left, since the console does nothing with backspaces for now
					// uncomment this else-if to see all special ASCII characters for debugging
					/*else if (c < 32 || c == 127)
						ctx.out << '(' << (int)c << ')';*/
					else
						ctx.out << c;
			}
			else if (nbytes < 0 && nbytes != LIBSSH2_ERROR_EAGAIN)
			{
				libssh2_session_last_error(session, &err_msg, NULL, 0);
				ctx.err << "Error reading from channel: " << err_msg << '\n';
				break;
			}

			// Don't process input if not focused
			if (!ctx.shell.IsFocused())
				continue;

			std::string seq;
			switch (const auto key = keyboardUpdate())
			{
			// make arrow keys work
			case DVK_UP:
				seq = "\e[A";
				break;
			case DVK_DOWN:
				seq = "\e[B";
				break;
			case DVK_RIGHT:
				seq = "\e[C";
				break;
			case DVK_LEFT:
				seq = "\e[D";
				break;
			case DVK_FOLD:
				seq = '\e';
				break;
			default:
				if (key > 0 && key < 128)
					seq = key;
			}

			if (seq.empty())
				continue;

			const auto nwritten = libssh2_channel_write(channel, seq.data(), seq.size());
			if (nwritten < 0 && nwritten != LIBSSH2_ERROR_EAGAIN)
			{
				libssh2_session_last_error(session, &err_msg, NULL, 0);
				ctx.err << "Error writing to channel: " << err_msg << '\n';
				break;
			}
		}
	} while (false);

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
