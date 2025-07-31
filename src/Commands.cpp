#include "Commands.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <nds.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace Commands
{

void help()
{
	auto num_commands = MAP.size();
	*Shell::out << num_commands << " commands available:\n";
	auto itr = MAP.cbegin();
	for (; num_commands > 1; --num_commands)
		*Shell::out << (itr++)->first << ' ';
	*Shell::out << itr->first << '\n';
}

void ls()
{
	const auto &path = (Shell::args.size() == 1) ? Shell::cwd : Shell::args[1];

	if (!fs::exists(path))
	{
		*Shell::err << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : fs::directory_iterator{path})
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			*Shell::out << "\e[44m" << filename << "\e[39m ";
		else
			*Shell::out << filename << ' ';
		*Shell::out << '\n';
	}
}

void cd()
{
	if (Shell::args.size() == 1)
	{
		Shell::cwd = "/";
		return;
	}

	const auto &path = Shell::args[1];

	if (!fs::exists(path))
	{
		*Shell::err << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	Shell::cwd = path;
}

void cat()
{
	if (Shell::args.size() == 1)
	{
		if (Shell::in == &std::cin)
		{
			// this gets you stuck in the cat command,
			// unless i make stdin nonblocking and use threads
			// so you can "ctrl+c" by pressing the fold key
			*Shell::err << "\e[41mcat: not using stdin\e[39m\n";
			return;
		}

		*Shell::out << Shell::in->rdbuf();
		return;
	}

	std::error_code ec;
	if (!fs::exists(Shell::args[1], ec) && !ec)
	{
		*Shell::err << "\e[41mcat: file does not exist: " << Shell::args[1]
					<< "\e[39m\n";
		return;
	}
	else if (ec)
	{
		*Shell::err << "\e[41mcat: " << ec.message() << "\e[39m\n";
		return;
	}

	const std::ifstream file{Shell::args[1]};

	if (!file)
	{
		*Shell::err << "\e[41mcat: cannot open file: " << Shell::args[1]
					<< "\e[39m\n";
		return;
	}

	*Shell::out << file.rdbuf();
}

void rm()
{
	if (Shell::args.size() == 1)
	{
		*Shell::err << "args: <filepaths...>\n";
		return;
	}

	for (auto itr = Shell::args.cbegin() + 1; itr < Shell::args.cend(); ++itr)
	{
		std::error_code ec;
		if (!fs::remove(Shell::args[1], ec))
			*Shell::err << "\e[41mrm: failed to remove: '" << Shell::args[1]
						<< "': " << ec.message() << "\e[39m\n";
	}
}

void echo()
{
	for (auto itr = Shell::args.cbegin() + 1; itr < Shell::args.cend(); ++itr)
		*Shell::out << *itr << ' ';
	*Shell::out << '\n';
}

void dns()
{
	if (Shell::args.size() == 1)
	{
		*Shell::err << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(Shell::args[1].c_str());
	if (!host)
	{
		*Shell::err << "\e[41mdns: gethostbyname: " << strerror(errno) << "\e[39m";
		return;
	}

	*Shell::out << *(in_addr *)host->h_addr_list[0] << '\n';
}

void ip()
{
	*Shell::out << (in_addr)Wifi_GetIP() << '\n';
}

void ipinfo()
{
	in_addr gateway, subnetMask, dns1, dns2;
	Wifi_GetIPInfo(&gateway, &subnetMask, &dns1, &dns2);

	*Shell::out << "IP:          " << (in_addr)Wifi_GetIP() << '\n'
				<< "Gateway:     " << gateway << '\n'
				<< "Subnet Mask: " << subnetMask << '\n'
				<< "DNS 1:       " << dns1 << '\n'
				<< "DNS 2:       " << dns2 << '\n';
}

void source()
{
	if (Shell::args.size() < 2)
	{
		*Shell::err << "args: <filepath>\n";
		return;
	}

	Shell::SourceFile(Shell::args[1]);
}

void rename()
{
	if (Shell::args.size() < 3)
	{
		*Shell::err << "args: <oldpath> <newpath>\n";
		return;
	}

	std::error_code ec;
	fs::rename(Shell::args[1], Shell::args[2], ec);
	if (ec)
		*Shell::err << "\e[41mrename: " << ec.message() << "\e[39m\n";
}

void pwd()
{
	*Shell::out << Shell::cwd << '\n';
}

void env()
{
	for (const auto &[k, v] : Shell::env)
		*Shell::out << k << '=' << v << '\n';
}

const std::unordered_map<std::string, void (*)()> MAP{
	{"help", help},
	{"echo", echo},
	{"cd", cd},
	{"ls", ls},
	{"cat", cat},
	{"rm", rm},
	{"dns", dns},
	{"wifi", wifi},
	{"ip", ip},
	{"ipinfo", ipinfo},
	{"tcp", tcp},
	{"curl", curl},
	{"pwd", pwd},
	{"env", env},
	{"rename", rename},
	{"clear", consoleClear},
	{"exit",
	 []
	 {
		 exit(0);
	 }},
	{"lua", lua},
	{"source", source}};

} // namespace Commands
