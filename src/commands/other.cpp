#include "Shell.hpp"

using namespace Shell;

// for printing ip addresses without first converting to a string
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip)
{
	return ostr << (ip.s_addr & 0xFF) << '.' << ((ip.s_addr >> 8) & 0xFF) << '.'
				<< ((ip.s_addr >> 16) & 0xFF) << '.'
				<< ((ip.s_addr >> 24) & 0xFF);
}

void Commands::help()
{
	std::cout << "commands: ";
	auto itr = Commands::map.cbegin();
	for (; itr != Commands::map.cend(); ++itr)
		std::cout << itr->first << ' ';
	std::cout << '\n';
}

void Commands::ls()
{
	const auto &path = (args.size() == 1) ? env["PWD"] : args[1];

	if (!std::filesystem::exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			std::cout << "\e[44m" << filename << "\e[39m ";
		else
			std::cout << filename << ' ';
		std::cout << '\n';
	}
}

void Commands::envCmd()
{
	for (const auto &[key, value] : env)
		std::cout << key << '=' << value << '\n';
}

void Commands::cd()
{
	if (args.size() == 1)
	{
		env["PWD"] = "/";
		return;
	}

	const auto &path = args[1];

	if (!std::filesystem::exists(path))
	{
		std::cerr << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	env["PWD"] = path;
}

void Commands::cat()
{
	if (args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!std::filesystem::exists(args[1]))
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

void Commands::rm()
{
	if (args.size() == 1)
	{
		std::cerr << "args: <filepath>\n";
		return;
	}

	if (!std::filesystem::exists(args[1]))
	{
		std::cerr << "file '" << args[1] << "' does not exist\n";
		return;
	}

	if (!std::filesystem::remove(args[1]))
		std::cerr << "failed to remove '" << args[1] << "'\n";
}

void Commands::echo()
{
	for (auto itr = args.cbegin() + 1; itr < args.cend(); ++itr)
		std::cout << *itr << ' ';
	std::cout << '\n';
}

// move this somewhere else later
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip);

void Commands::dns()
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

	std::cout << *(in_addr *)host->h_addr_list[0] << '\n';
}
