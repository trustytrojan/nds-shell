#include "CliPrompt.hpp"
#include "Commands.hpp"
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

	*Shell::out << "press fold/esc key to exit\n";

	CliPrompt prompt;
	prompt.setOutputStream(Shell::out);
	prompt.setPrompt("lua> ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	while (pmMainLoop())
	{
		swiWaitForVBlank();
		prompt.processKeyboard();

		if (prompt.enterPressed())
		{
			auto line = prompt.getInput();

			if (!line.contains("print") && !line.contains("return"))
				line = "return " + line;

			*Shell::out << lua.safe_script(line).get<sol::object>() << '\n';

			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}

		if (prompt.foldPressed())
		{
			*Shell::out << "\r\e[2K";
			break;
		}
	}
}
