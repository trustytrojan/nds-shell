#include "CliPrompt.hpp"
#include "EscapeSequences.hpp"

#include <iostream>

using namespace EscapeSequences;

CliPrompt::CliPrompt(
	const std::string &prompt, const char cursor, std::ostream &ostr)
	: prompt{prompt},
	  cursor{cursor},
	  ostr{ostr}
{
}

void CliPrompt::GetLine(std::string &line)
{
	line.clear();
	resetProcessKeyboardState();
	std::cout << prompt << cursor << Cursor::moveLeftOne;
	while (pmMainLoop() && !newlineEntered)
	{
		ProcessKeyboard(line);
		swiWaitForVBlank();
	}
}

void CliPrompt::ProcessKeyboard(std::string &line)
{
	/**
	 * While `cursorPos != line.size()`, the character pointed to by the cursor
	 * will flash between `line[cursorPos]` and `this->cursor`.
	 */
	static const auto FLASH_INTERVAL = 10;

	switch (s8 c = keyboardUpdate())
	{
	// keys to ignore:
	case 0:
	case -1:
	case DVK_CTRL:
	case DVK_ALT:
	case DVK_CAPS:
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
			((flashState = !flashState) ? (ostr << cursor)
										: (ostr << line[cursorPos]))
				<< Cursor::moveLeftOne;
		}
		break;

	case DVK_ENTER:
		if (cursorPos == line.size())
			ostr << ' ';
		else
			ostr << line[cursorPos] << '\r';
		ostr << '\n';
		newlineEntered = true;
		break;

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
			ostr << "\b\e[0K" << std::string_view{line}.substr(cursorPos)
				 << Cursor::move(
						Cursor::MoveDirection::LEFT, line.size() - cursorPos);
		}
		break;

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

	case DVK_RIGHT:
		if (cursorPos == line.size())
			break;
		ostr << line[cursorPos];
		if (++cursorPos == line.size())
			ostr << cursor << Cursor::moveLeftOne;
		break;

	default:
		if (cursorPos == line.size())
			line += c;
		else
			line.insert(line.begin() + cursorPos, c);
		ostr << c;
		if (++cursorPos == line.size())
			ostr << cursor << Cursor::moveLeftOne;
	}
}
