#include "my_state.hpp"

void my_state::load_CliPrompt()
{
	// clang-format off
	new_usertype<CliPrompt>("CliPrompt",
		"new", sol::constructors<CliPrompt()>{},
		"new", sol::factories([&](const std::string &prompt) { return CliPrompt{prompt, ctx.out}; }),
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
	// clang-format on
}
