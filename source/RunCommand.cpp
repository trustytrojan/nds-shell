#include "everything.hpp"

void exit(const Args &, const StandardStreams &);
void help(const Args &, const StandardStreams &);

void ls(const Args &, const StandardStreams &);
void rm(const Args &, const StandardStreams &);
void cat(const Args &, const StandardStreams &);
void echo(const Args &, const StandardStreams &);
void clear(const Args &, const StandardStreams &);

void batlvl(const Args &, const StandardStreams &);
void wifi(const Args &, const StandardStreams &);
void dns(const Args &, const StandardStreams &);
void time(const Args &, const StandardStreams &);
void http(const Args &, const StandardStreams &);

const std::unordered_map<std::string, CommandFunction> commands {
	{"exit", exit},
	{"help", help},

	{"cat", cat},
	{"ls", ls},
	{"rm", rm},
	{"echo", echo},
	{"clear", clear},

	{"wifi", wifi},
	{"batlvl", batlvl},
	{"dns", dns},
	{"time", time},
	{"http", http}
};

const auto commandsEnd = commands.cend();

void help(const Args &, const StandardStreams &stdio)
{
	*stdio.out << "commands: ";
	auto itr = commands.cbegin();
	for (; itr != commandsEnd; ++itr)
		*stdio.out << itr->first << ' ';
	*stdio.out << '\n';
}

void RunCommand(const Args &args, const StandardStreams &stdio)
{
	const auto command = args.front();
	const auto itr = commands.find(command);
	if (itr == commandsEnd)
		std::cerr << "\e[43munknown command\e[39m\n";
	else
		itr->second(args, stdio);
}
