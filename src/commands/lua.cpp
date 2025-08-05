#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "Consoles.hpp"
// #include "Shell.hpp"

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

void Commands::lua(const Context &ctx)
{
	sol::state lua;
	lua.open_libraries();

	if (ctx.args.size() > 1)
	{
		lua.safe_script_file(ctx.args[1]);
		return;
	}

	ctx.out << "press fold/esc key to exit\n";

	CliPrompt prompt;
	prompt.setOutputStream(ctx.out);
	prompt.setPrompt("lua> ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	while (pmMainLoop())
	{
		threadYield();

		if (ctx.shell.console != Consoles::focused_console)
			continue;

		prompt.processKeyboard();

		if (prompt.enterPressed())
		{
			auto line = prompt.getInput();

			if (!line.contains("print") && !line.contains("return"))
				line = "return " + line;

			ctx.out << lua.safe_script(line).get<sol::object>() << '\n';

			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}

		if (prompt.foldPressed())
		{
			ctx.out << "\r\e[2K";
			break;
		}
	}
}
