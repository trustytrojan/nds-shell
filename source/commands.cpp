#include "everything.hpp"
#include "NetParse.hpp"

void exit(const Args &, const StandardStreams &)
{
	systemShutDown();
}

void clear(const Args &, const StandardStreams &stdio)
{
	*stdio.out << "\e[2J";
}

void ls(const Args &args, const StandardStreams &stdio)
{
	const auto path = (args.size() == 1) ? "/" : args[1];

	if (!std::filesystem::exists(path))
	{
		*stdio.err << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			*stdio.out << "\e[44m" << filename << "\e[39m";
		else
			*stdio.out << filename;
		*stdio.out << '\n';
	}
}

void cat(const Args &args, const StandardStreams &stdio)
{
	if (args.size() == 1)
	{
		*stdio.err << "args: <filepath>\n";
		return;
	}

	if (!std::filesystem::exists(args[1]))
	{
		*stdio.err << "file '" << args[1] << "' does not exist\n";
		return;
	}

	std::ifstream file(args[1]);

	if (!file)
	{
		*stdio.err << "cannot open file\n";
		return;
	}

	*stdio.out << file.rdbuf();
	file.close();
}

void rm(const Args &args, const StandardStreams &stdio)
{
	if (args.size() == 1)
	{
		*stdio.err << "args: <filepath>\n";
		return;
	}

	if (!std::filesystem::exists(args[1]))
	{
		*stdio.err << "file '" << args[1] << "' does not exist\n";
		return;
	}

	if (!std::filesystem::remove(args[1]))
	{
		*stdio.err << "failed to remove '"<< args[1] << "'\n";
	}
}

void echo(const Args &args, const StandardStreams &stdio)
{
	for (auto itr = args.cbegin() + 1; itr != args.cend(); ++itr)
		*stdio.out << *itr << ' ';
	*stdio.out << '\n';
}

void batlvl(const Args &args, const StandardStreams &stdio)
{
	*stdio.out << getBatteryLevel() << '\n';
}

// unfortunately i don't think `system_clock::now` returns anything correct
// will have to steal example C code from libnds
void time(const Args &args, const StandardStreams &stdio)
{
	static const char *const monthStr[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	static const char *const weekdayStr[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	using namespace std::chrono;

	const auto tt = system_clock::to_time_t(system_clock::now());
	*stdio.out << tt << '\n';
	const auto tm = gmtime(&tt);
	*stdio.out << weekdayStr[tm->tm_wday] << ' ' << tm->tm_mday << ' ' << monthStr[tm->tm_mon] << ' ' << tm->tm_year << '\n'
			  << tm->tm_hour << ':' << tm->tm_min << ':' << tm->tm_sec << '\n';
}

void dns(const Args &args, const StandardStreams &stdio)
{
	if (args.size() == 1)
	{
		*stdio.err << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(args[1].c_str());
	if (!host)
	{
		perror("gethostbyname");
		return;
	}

	*stdio.out << *(in_addr *)host->h_addr_list[0] << '\n';
}
