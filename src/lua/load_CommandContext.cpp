#include "my_state.hpp"

void my_state::load_CommandContext()
{
	// clang-format off
	using CC = Commands::Context;
	new_usertype<CC>("CommandContext",
		sol::no_constructor,
		"shell", sol::readonly_property([](const CC &c) { return std::ref(c.shell); }),
		"args", sol::readonly_property([](const CC &c) { return std::ref(c.args); }),
		"out", sol::readonly_property([](const CC &c) { return std::ref(c.out); }),
		"err", sol::readonly_property([](const CC &c) { return std::ref(c.err); }),
		"GetEnv", &CC::GetEnv
	);
	// clang-format on
}
