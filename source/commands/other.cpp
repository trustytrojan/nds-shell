#include "../NDS_Shell.hpp"
#include "../NetUtils.hpp"

using namespace NDS_Shell;

void Commands::help()
{
	*stdio.out << "commands: ";
	auto itr = Commands::map.cbegin();
	for (; itr != Commands::map.cend(); ++itr)
		*stdio.out << itr->first << ' ';
	*stdio.out << '\n';
}

void Commands::ls()
{
	const auto &path = (args.size() == 1) ? env["PWD"] : args[1];

	if (!std::filesystem::exists(path))
	{
		*stdio.err << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : std::filesystem::directory_iterator(path))
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			*stdio.out << "\e[44m" << filename << "\e[39m ";
		else
			*stdio.out << filename << ' ';
		*stdio.out << '\n';
	}
}

void Commands::envCmd()
{
	for (const auto &[key, value] : env)
		*stdio.out << key << '=' << value << '\n';
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
		*stdio.err << "\e[41mpath does not exist\e[39m\n";
		return;
	}

	env["PWD"] = path;
}

void Commands::cat()
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

void Commands::rm()
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
		*stdio.err << "failed to remove '" << args[1] << "'\n";
	}
}

void Commands::echo()
{
	for (auto itr = args.cbegin() + 1; itr < args.cend(); ++itr)
		*stdio.out << *itr << ' ';
	*stdio.out << '\n';
}

// move this somewhere else later
std::ostream &operator<<(std::ostream &ostr, const in_addr &ip);

void Commands::dns()
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
