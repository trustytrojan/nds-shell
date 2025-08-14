#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "my_state.hpp"

#include <curl/curl.h>
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
	my_state lua{ctx};

	if (ctx.args.size() > 1)
	{
		const auto &result = lua.safe_script_file(ctx.args[1]);
		if (!result.valid())
		{
			sol::error err = result;
			ctx.err << "\e[91mlua: " << err.what() << "\e[39m\n";
		}
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
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		if (!ctx.shell.IsFocused())
			continue;

		prompt.update();

		if (prompt.enterPressed())
		{
			auto line = prompt.getInput();

			if (!line.contains("print") && !line.contains("return"))
				line = "return " + line;

			const auto result = lua.safe_script(line);
			if (!result)
			{
				sol::error err = result;
				ctx.err << "\e[91mlua: " << err.what() << "\e[39m\n";
			}
			else
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
