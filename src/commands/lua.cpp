#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "EscapeSequences.hpp"
#include "Shell.hpp"

#include <sol/sol.hpp>

static std::string
lua_value_to_string(const sol::object &obj, const bool expand_function = true)
{
	std::ostringstream ss;
	switch (obj.get_type())
	{
	case sol::type::nil:
		ss << "nil";
		break;
	case sol::type::table:
		ss << '{';
		for (const auto &[k, v] : obj.as<sol::table>())
			ss << lua_value_to_string(k) << ": "
			   << lua_value_to_string(v, false) << ", ";
		ss << '}';
		break;
	case sol::type::function:
		ss << "function";
		if (expand_function)
			ss << ": " << obj.as<sol::function>().pointer();
		break;
	default:
		ss << obj.as<std::string>();
		break;
	}
	return ss.str();
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

			std::cout << lua_value_to_string(lua.safe_script(line)) << '\n';

			line.clear();
			prompt.resetProcessKeyboardState();
			std::cout << prompt.prompt << prompt.cursor
					  << EscapeSequences::Cursor::moveLeftOne;
			;
		}

		if (prompt.foldPressed())
		{
			std::cout << "\r\e[2K";
			break;
		}
	}
}
