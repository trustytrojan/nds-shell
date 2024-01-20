#include "everything.hpp"
#include "NetParse.hpp"

void exit(const Args &args)
{
	systemShutDown();
}

void ls(const Args &args)
{
	using namespace std::filesystem;

	const auto path = (args.size() == 1) ? "/" : args[1];

	if (!exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			std::cout << "\e[44m" << filename << "\e[39m";
		else if (filename.substr(filename.size() - 4) == ".nds")
			std::cout << "\e[42m" << filename << "\e[39m";
		else
			std::cout << filename;
		std::cout << '\n';
	}
}

void cat(const Args &args)
{
	if (args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	using namespace std::filesystem;

	if (!exists(args[1]))
	{
		std::cerr << "file '" << args[1] << "' does not exist\n";
		return;
	}

	std::ifstream file(args[1]);

	if (!file)
	{
		std::cerr << "cannot open file\n";
		return;
	}

	std::cout << file.rdbuf();
	file.close();
}

void batlvl(const Args &args)
{
	std::cout << getBatteryLevel() << '\n';
}

// unfortunately i don't think `system_clock::now` returns anything correct
// will have to steal example C code from libnds
void time(const Args &args)
{
	static const char *const monthStr[] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
	static const char *const weekdayStr[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
	using namespace std::chrono;

	const auto tt = system_clock::to_time_t(system_clock::now());
	std::cout << tt << '\n';
	const auto tm = gmtime(&tt);
	std::cout << weekdayStr[tm->tm_wday] << ' ' << tm->tm_mday << ' ' << monthStr[tm->tm_mon] << ' ' << tm->tm_year << '\n'
			  << tm->tm_hour << ':' << tm->tm_min << ':' << tm->tm_sec << '\n';
}

void dns(const Args &args)
{
	if (args.size() == 1)
	{
		std::cerr << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(args[1].c_str());
	if (!host)
	{
		perror("gethostbyname");
		return;
	}

	std::cout << inet_ntoa(*(in_addr *)host->h_addr_list[0]) << '\n';
}
