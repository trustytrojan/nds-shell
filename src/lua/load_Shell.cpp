#include "my_state.hpp"

void my_state::load_Shell()
{
	// clang-format off
	new_usertype<Shell>("Shell",
		sol::no_constructor,
		"focused", sol::readonly_property(&Shell::IsFocused),
		"run", [&](Shell &self, const sol::string_view &line)
		{
			// of course, if the line has an i/o redirect,
			// that will be applied by Shell::ProcessLine, so the
			// stringstream may receive nothing, which the user wanted.
			std::ostringstream ss;
			self.RedirectOutput(1, ss);
			self.RedirectOutput(2, ss);
			self.ProcessLine(line);
			return ss.str();
		}
	);
	// clang-format on
}
