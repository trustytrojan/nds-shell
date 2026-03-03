#include "my_state.hpp"
#include <sol/property.hpp>

void my_state::load_Shell()
{
	// clang-format off
	new_usertype<Shell>("Shell",
		sol::no_constructor,
		"focused",
#ifdef NDSH_MULTICONSOLE
		sol::readonly_property(&Shell::IsFocused)
#else
		sol::readonly_property([] { return true; })
#endif
		,
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
