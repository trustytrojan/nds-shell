#include "everything.hpp"

void exit(const Args &args);
void help(const Args &args);
void ls(const Args &args);
void batlvl(const Args &args);
void wifi(const Args &args);
void dns(const Args &args);
void time(const Args &args);
void cat(const Args &args);
void http(const Args &args);

const std::unordered_map<std::string, CommandFunction> commands{
	{"help", help},
	{"wifi", wifi},
	{"ls", ls},
	{"batlvl", batlvl},
	{"exit", exit},
	{"dns", dns},
	{"time", time},
	{"cat", cat},
	{"http", http}};

const auto commandsEnd = commands.cend();

void help(const Args &args)
{
	std::cout << "commands: ";
	auto itr = commands.cbegin();
	for (; itr != commandsEnd; ++itr)
		std::cout << itr->first << ' ';
	std::cout << '\n';
}

void RunCommand(const Args &args)
{
	const auto command = args.front();
	const auto itr = commands.find(command);
	if (itr == commandsEnd)
		std::cerr << "\e[43munknown command\e[39m\n";
	else
		itr->second(args);
}
