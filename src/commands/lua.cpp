#include "CliPrompt.hpp"
#include "Commands.hpp"

#include <sol/sol.hpp>
#include <sstream>

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

void setup_lua_state(sol::state &lua, const Commands::Context &ctx)
{
	lua.open_libraries(sol::lib::string);

	// clang-format off
	lua.create_named_table("libnds",
		"set_brightness", setBrightness
	);
	// clang-format on

	lua.new_usertype<Shell>(
		"Shell",
		sol::no_constructor,

		// Expose the IsFocused method as a readonly property
		"is_focused",
		sol::readonly_property(&Shell::IsFocused),

		// Keep your custom 'run' command logic
		"run",
		[&](Shell &self, const std::string &line)
		{
			// of course, if the line has an i/o redirect,
			// that will be applied by Shell::ProcessLine, so the
			// stringstream may receive nothing, which the user wanted.
			std::ostringstream ss;
			self.RedirectOutput(1, ss);
			self.RedirectOutput(2, ss);
			self.ProcessLine(line);
			return ss.str();
		});

	// clang-format off
	lua.create_named_table("ctx",
		"args", ctx.args,
		"shell", ctx.shell
	);
	// clang-format on

	lua["print"] = [&](const sol::string_view &s)
	{
		ctx.out << s << '\n';
	};
}

void Commands::lua(const Context &ctx)
{
	sol::state lua;
	setup_lua_state(lua, ctx);

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
