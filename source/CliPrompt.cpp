#include "CliPrompt.hpp"
#include "EscapeSequences.hpp"

using namespace EscapeSequences;

CliPrompt::CliPrompt(const std::string &bp, const char c, std::ostream &o)
	: basePrompt(bp), cursor(c), ostr(o) {}

void CliPrompt::GetLine(std::string &line) const
{
	// State necessary for processKeyboard
	u32 cursorPos = 0;
	bool flashState = false;
	u8 flashTimer = 0;
	bool newlineEntered = false;

	// Print initial prompt
	std::cout << basePrompt << cursor << Cursor::moveLeftOne;

	// Process the keyboard state at the DS's refresh rate
	while (!newlineEntered)
	{
		ProcessKeyboard(line, cursorPos, flashState, flashTimer, newlineEntered);
		swiWaitForVBlank();
	}
}

void CliPrompt::ProcessKeyboard(std::string &line, u32 &cursorPos, bool &flashState, u8 &flashTimer, bool &newlineEntered) const
{
	/**
	 * While `cursorPos != line.size()`, the character pointed to by the cursor
	 * will flash between `line[cursorPos]` and `this->cursor`.
	 */
	static const auto FLASH_INTERVAL = 10;

	switch (s8 c = keyboardUpdate())
	{
	// no keys pressed, or ignored keys
	case 0:
	case -1:
	case DVK_CTRL:
	case DVK_ALT:
	case DVK_CAPS:
	case DVK_DOWN:
	case DVK_UP:
	case DVK_FOLD:
	case DVK_MENU:
	case DVK_SHIFT:
		if (cursorPos == line.size())
		{
			flashTimer = 0;
			flashState = false;
		}
		else if (++flashTimer >= FLASH_INTERVAL)
		{
			flashTimer = 0;
			((flashState = !flashState) ? (ostr << cursor) : (ostr << line[cursorPos])) << Cursor::moveLeftOne;
		}
		break;

	// enter, newline, '\n', 10
	case DVK_ENTER:
		if (cursorPos == line.size())
			ostr << ' ';
		else
			ostr << line[cursorPos] << '\r';
		ostr << '\n';
		newlineEntered = true;
		break;

	// backspace, '\b', 8
	case DVK_BACKSPACE:
		if (line.empty() || cursorPos == 0)
			break;
		if (cursorPos == line.size())
		{
			line.erase(--cursorPos, 1);
			ostr << " \b\b" << cursor << Cursor::moveLeftOne;
		}
		else
		{
			line.erase(--cursorPos, 1);
			ostr << "\b\e[0K" << line.substr(cursorPos) << Cursor::move(Cursor::MoveDirection::LEFT, line.size() - cursorPos);
		}
		break;

	// left arrow
	case DVK_LEFT:
		if (cursorPos == 0)
			break;
		if (cursorPos == line.size())
			ostr << " \b";
		if (flashState)
			ostr << line[cursorPos] << Cursor::moveLeftOne;
		ostr << Cursor::moveLeftOne;
		--cursorPos;
		break;

	// right arrow
	case DVK_RIGHT:
		if (cursorPos == line.size())
			break;
		ostr << line[cursorPos];
		if (++cursorPos == line.size())
			ostr << cursor << Cursor::moveLeftOne;
		break;

	// any other valid character
	default:
		if (cursorPos == line.size())
			line += c;
		else
			line.insert(line.begin() + cursorPos, c);
		// line[cursorPos] = c;
		ostr << c;
		if (++cursorPos == line.size())
			ostr << cursor << Cursor::moveLeftOne;
	}

	// just in case
	assert(cursorPos >= 0 && cursorPos <= line.size());
}