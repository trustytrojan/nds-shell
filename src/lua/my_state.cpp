#include "my_state.hpp"

my_state::my_state(const Commands::Context &ctx)
	: ctx{ctx}
{
	// we will load all stdlibs, it is UP TO THE USER to not call anything
	// that will break the whole system!!!!!!!!!!
	open_libraries();

	load_libnds_funcs();
	load_CliPrompt();
	load_Shell();
	load_CommandContext();
	load_fetch();

	set("ctx", ctx);
	set("print", [&](const sol::string_view &s) { ctx.out << s << '\n'; });
	set("printerr", [&](const sol::string_view &s) { ctx.err << s << '\n'; });
}
