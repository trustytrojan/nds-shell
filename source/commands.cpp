#include "everything.hpp"

void help(const Args &args);
void exit(const Args &args);
void ls(const Args &args);
void wifi(const Args &args);

const std::unordered_map<std::string, CommandFunction> commands = {
	{"help", help},
	{"exit", exit},
	{"wifi", wifi},
	{"ls", ls}
};

void help(const Args &args)
{
	std::cout << "commands: ";
	auto itr = commands.cbegin();
	for (; itr != commands.cend(); ++itr)
		std::cout << itr->first << ' ';
	std::cout << '\n';
}

void exit(const Args &args)
{
	exit(0);
}

void ls(const Args &args)
{
	for (const auto &entry : std::filesystem::directory_iterator("/"))
	{
		std::cout << entry.path().filename() << '\n';
	}
}

const auto commandsEnd = commands.cend();

void RunCommand(const Args &args)
{
	const auto command = args.front();
	const auto itr = commands.find(command);
	if (itr == commandsEnd)
		std::cout << "unknown command\n";
	else
		itr->second(args);
}
