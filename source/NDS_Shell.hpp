#ifndef __NDS_SHELL__
#define __NDS_SHELL__

#include "libdeps.hpp"
#include "StandardStreams.hpp"

namespace NDS_Shell
{
	using Args = std::vector<std::string>;
	using Command = std::function<void(const Args &, const StandardStreams &)>;

	void Init();
	void Start();
	void ProcessLine(const std::string &line);
	void RunCommand(const Args &args, const StandardStreams &stdio);

	// environment variables are wip!
	std::unordered_map<std::string, std::string> env {
		{"PWD", "/"},
		{"PS1", "> "}
	};

	namespace Commands
	{
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

		const std::unordered_map<std::string, Command> map {
			{"exit", exit},
			{"help", help},
			{"ls", ls},
			{"rm", rm},
			{"cat", cat},
			{"echo", echo},
			{"clear", clear},
			{"batlvl", batlvl},
			{"wifi", wifi},
			{"dns", dns},
			{"time", time},
			{"http", http}
		};

		const auto begin = map.cbegin();
		const auto end = map.cend();
	}
};

#endif