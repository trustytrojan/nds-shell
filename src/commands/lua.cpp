#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "EscapeSequences.hpp"
#include "Shell.hpp"

#include <sol/sol.hpp>

std::ostream &operator<<(std::ostream &ostr, const sol::object &obj)
{
	switch (obj.get_type())
	{
	case sol::type::nil:
		return ostr << "nil";
	case sol::type::table:
		return ostr << "table: " << obj.pointer();
	case sol::type::function:
		return ostr << "function: " << obj.pointer();
	default:
		return ostr << obj.as<std::string_view>();
	}
}

void Commands::lua()
{
	sol::state lua;
	lua.open_libraries();

	if (Shell::args.size() > 1)
	{
		lua.safe_script_file(Shell::args[1]);
		return;
	}

	CliPrompt prompt{"lua> ", '_', std::cout};
	std::string line;
	prompt.resetProcessKeyboardState();
	std::cout << prompt.prompt << prompt.cursor
			  << EscapeSequences::Cursor::moveLeftOne;

	while (pmMainLoop())
	{
		swiWaitForVBlank();
		prompt.processKeyboard(line);

		if (prompt.newlineEntered())
		{
			if (!line.contains("print") && !line.contains("return"))
				line = "return " + line;

			std::cout << lua.safe_script(line).get<sol::object>() << '\n';

			line.clear();
			prompt.resetProcessKeyboardState();
			std::cout << prompt.prompt << prompt.cursor
					  << EscapeSequences::Cursor::moveLeftOne;
		}

		if (prompt.foldPressed())
		{
			std::cout << "\r\e[2K";
			break;
		}
	}
}
