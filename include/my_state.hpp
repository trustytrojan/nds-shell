#pragma once

#include "Commands.hpp"

#include <sol/sol.hpp>

struct my_state : sol::state
{
	const Commands::Context &ctx;
	my_state(const Commands::Context &);
	void load_libnds_funcs();
	void load_CliPrompt();
	void load_Shell();
	void load_CommandContext();
	void load_fetch();
};
