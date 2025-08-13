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

struct my_state : sol::state
{
	my_state(const Commands::Context &ctx);
};

my_state::my_state(const Commands::Context &ctx)
{
	open_libraries(sol::lib::string);

	// clang-format off
	create_named_table("libnds",
		"setBrightness", setBrightness, // this was just for testing, leave it in anyway for fun
		"pmMainLoop", pmMainLoop,
		"threadYield", threadYield
	);

	new_usertype<CliPrompt>("CliPrompt",
		"prompt", sol::writeonly_property(&CliPrompt::setPrompt),
		"ostr", sol::writeonly_property(&CliPrompt::setOutputStream),
		"input", sol::readonly_property(&CliPrompt::getInput),
		"printFullPrompt", &CliPrompt::printFullPrompt,
		"prepareForNextLine", &CliPrompt::prepareForNextLine,
		"update", &CliPrompt::update,
		"enterPressed", sol::readonly_property(&CliPrompt::enterPressed),
		"foldPressed", sol::readonly_property(&CliPrompt::foldPressed),
		"lineHistory", sol::readonly_property(&CliPrompt::getLineHistory),
		"setLineHistoryFromFile", &CliPrompt::setLineHistoryFromFile,
		"clearLineHistory", &CliPrompt::clearLineHistory
	);

	new_usertype<Shell>("Shell",
		sol::no_constructor,
		"focused", sol::readonly_property(&Shell::IsFocused),
		"run", [&](Shell &self, const std::string &line)
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
	
	new_usertype<Commands::Context>("CommandContext",
		sol::no_constructor,
		"shell", sol::readonly_property([](const Commands::Context &c) { return std::ref(c.shell); }),
		"args", sol::readonly_property([](const Commands::Context &c) { return std::ref(c.args); }),
		"out", sol::readonly_property([](const Commands::Context &c) { return std::ref(c.out); }),
		"err", sol::readonly_property([](const Commands::Context &c) { return std::ref(c.err); }),
		"GetEnv", &Commands::Context::GetEnv
	);
	// clang-format on

	set("ctx", ctx);
	set("print", [&](const sol::string_view &s) { ctx.out << s << '\n'; });
	set("printerr", [&](const sol::string_view &s) { ctx.err << s << '\n'; });
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
