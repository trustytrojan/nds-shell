#include "Commands.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
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
	std::cout << "commands: ";
	auto itr = MAP.cbegin();
	for (; itr != MAP.cend(); ++itr)
		std::cout << itr->first << ' ';
	std::cout << '\n';
}

void ls()
{
	const auto &path = (Shell::args.size() == 1) ? Shell::cwd : Shell::args[1];

	if (!fs::exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : fs::directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			std::cout << "\e[44m" << filename << "\e[39m ";
		else
			std::cout << filename << ' ';
		std::cout << '\n';
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
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	Shell::cwd = path;
}

void cat()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!fs::exists(Shell::args[1]))
	{
		std::cerr << "file '" << Shell::args[1] << "' does not exist\n";
		return;
	}

	std::ifstream file(Shell::args[1]);

	if (!file)
	{
		std::cerr << "cannot open file\n";
		return;
	}

	std::cout << file.rdbuf();
	file.close();
}

void rm()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!fs::exists(Shell::args[1]))
	{
		std::cerr << "file '" << Shell::args[1] << "' does not exist\n";
		return;
	}

	if (!fs::remove(Shell::args[1]))
		std::cerr << "failed to remove '" << Shell::args[1] << "'\n";
}

void echo()
{
	for (auto itr = Shell::args.cbegin() + 1; itr < Shell::args.cend(); ++itr)
		std::cout << *itr << ' ';
	std::cout << '\n';
}

void dns()
{
	if (Shell::args.size() == 1)
	{
		std::cerr << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(Shell::args[1].c_str());
	if (!host)
	{
		perror("gethostbyname");
		return;
	}

	std::cout << *(in_addr *)host->h_addr_list[0] << '\n';
}

} // namespace Commands
