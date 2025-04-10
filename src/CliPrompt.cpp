#include "CliPrompt.hpp"
#include "EscapeSequences.hpp"

using namespace EscapeSequences;

CliPrompt::CliPrompt(const std::string &bp, const char c, std::ostream &o)
	: basePrompt(bp),
	  cursor(c),
	  ostr(o)
{
}

void CliPrompt::GetLine(std::string &line)
{
	// For convenience
	line.clear();

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
		ProcessKeyboard(
			line, cursorPos, flashState, flashTimer, newlineEntered);
		swiWaitForVBlank();
	}
}

void CliPrompt::ProcessKeyboard(
	std::string &line,
	u32 &cursorPos,
	bool &flashState,
	u8 &flashTimer,
	bool &newlineEntered)
{
	/**
	 * While `cursorPos != line.size()`, the character pointed to by the cursor
	 * will flash between `line[cursorPos]` and `this->cursor`.
	 */
	static const auto FLASH_INTERVAL = 10;

	switch (s8 c = keyboardUpdate())
	{
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
		lineHistory.push_back(line);
		lineHistoryItr = lineHistory.end() - 1;
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
			ostr << "\b\e[0K" << line.substr(cursorPos)
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

	case DVK_UP:
		line = lineHistory.back();
		cursorPos = line.size();
		break;

	case DVK_DOWN:
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
