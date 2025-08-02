#include "CliPrompt.hpp"
#include "EscapeSequences.hpp"

#include <fstream>
#include <iostream>

using namespace EscapeSequences;

CliPrompt::CliPrompt(
	const std::string &prompt, const char cursor, std::ostream &ostr)
	: prompt{prompt},
	  cursor{cursor},
	  ostr{ostr}
{
}

void CliPrompt::getLine(std::string &line)
{
	line.clear();
	resetProcessKeyboardState();
	ostr << prompt << cursor << Cursor::moveLeftOne;
	do
	{
		processKeyboard(line);
		swiWaitForVBlank();
	} while (pmMainLoop() && !_newlineEntered);
}

void CliPrompt::resetProcessKeyboardState()
{
	cursorPos = flashState = flashTimer = {};
	lineHistoryItr = lineHistory.cend();
	savedInputLine.clear();
}

void CliPrompt::flashCursor(const std::string &line)
{
	/**
	 * While `cursorPos != line.size()`, the character pointed to by the cursor
	 * will flash between `line[cursorPos]` and `this->cursor`.
	 */
	static const auto FLASH_INTERVAL = 10;

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
}

void CliPrompt::handleBackspace(std::string &line)
{
	if (line.empty() || cursorPos == 0)
		return;
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
}

void CliPrompt::handleEnter(const std::string &line)
{
	if (cursorPos == line.size())
		ostr << ' ';
	else
		ostr << line[cursorPos] << '\r';

	ostr << '\n';
	_newlineEntered = true;

	// don't push empty lines!
	if (line.size())
		lineHistory.emplace_back(line);
}

void CliPrompt::handleLeft(const std::string &line)
{
	if (line.length() == 0 || cursorPos == 0)
		return;
	if (cursorPos == line.size())
		ostr << " \b";
	if (flashState)
		ostr << line[cursorPos] << Cursor::moveLeftOne;
	ostr << Cursor::moveLeftOne;
	--cursorPos;
}

void CliPrompt::handleRight(const std::string &line)
{
	if (line.length() == 0 || cursorPos == line.size())
		return;
	ostr << line[cursorPos];
	if (++cursorPos == line.size())
		ostr << cursor << Cursor::moveLeftOne;
}

void CliPrompt::handleUp(std::string &line)
{
	if (lineHistory.empty() || lineHistoryItr - 1 < lineHistory.cbegin())
		// obviously do nothing here
		return;

	if (lineHistoryItr == lineHistory.cend())
		// save the "new" line for when the user goes all the way down again
		savedInputLine = line;

	// go up one, reset cursorPos, reprint everything
	line = *--lineHistoryItr;
	cursorPos = line.size();
	ostr << "\r\e[2K" << prompt << line << cursor << Cursor::moveLeftOne;
}

void CliPrompt::handleDown(std::string &line)
{
	if (lineHistory.empty() || lineHistoryItr + 1 > lineHistory.cend())
		// obviously do nothing here
		return;

	// go down one
	++lineHistoryItr;

	if (lineHistoryItr == lineHistory.cend())
	{
		// we're at the "new" line, restore it
		line = savedInputLine;
		cursorPos = line.size();
		ostr << "\r\e[2K" << prompt << line << cursor << Cursor::moveLeftOne;
		return;
	}

	// go down one, reset cursorPos, reprint everything
	line = *lineHistoryItr;
	cursorPos = line.size();
	ostr << "\r\e[2K" << prompt << line << cursor << Cursor::moveLeftOne;
}

void CliPrompt::processKeyboard(std::string &line)
{
	resetKeypressState();

	scanKeys();
	const auto keysPressed = keysDownRepeat();
	if (keysPressed & (KEY_A | KEY_START))
		handleEnter(line);
	if (keysPressed & KEY_B)
		handleBackspace(line);
	if (keysPressed & KEY_LEFT)
		handleLeft(line);
	if (keysPressed & KEY_RIGHT)
		handleRight(line);
	if (keysPressed & KEY_UP)
		handleUp(line);
	if (keysPressed & KEY_DOWN)
		handleDown(line);

	switch (const s8 c = keyboardUpdate())
	{
	case 0: // just in case
	case NOKEY:
	case DVK_TAB: // ignore tabs for now, use them for autocomplete later
	case DVK_CTRL:
	case DVK_ALT:
	case DVK_CAPS:
	case DVK_MENU:
	case DVK_SHIFT:
		flashCursor(line);
		break;

	case DVK_FOLD:
		_foldPressed = true;
		break;

	case DVK_ENTER:
		if (keysPressed & KEY_A)
			break;
		handleEnter(line);
		break;

	case DVK_BACKSPACE:
		if (keysPressed & KEY_B)
			break;
		handleBackspace(line);
		break;

	case DVK_LEFT:
		if (keysPressed & KEY_LEFT)
			break;
		handleLeft(line);
		break;

	case DVK_RIGHT:
		if (keysPressed & KEY_RIGHT)
			break;
		handleRight(line);
		break;

	case DVK_UP:
		if (keysPressed & KEY_UP)
			break;
		handleUp(line);
		break;

	case DVK_DOWN:
		if (keysPressed & KEY_DOWN)
			break;
		handleDown(line);
		break;

	default:
		if (cursorPos == line.size())
		{
			line += c;
			ostr << c;
		}
		else
		{
			line.insert(line.begin() + cursorPos, c);
			ostr << line.c_str() + cursorPos
				 << Cursor::move(
						Cursor::MoveDirection::LEFT,
						line.size() - cursorPos - 1);
		}
		if (++cursorPos == line.size())
			ostr << cursor << Cursor::moveLeftOne;
	}
}

void CliPrompt::setLineHistory(const std::string &filename)
{
	lineHistory.clear();

	std::ifstream lineHistoryFile{filename};
	if (!lineHistoryFile)
	{
		std::cerr << "\e[41mfailed to load line history\n\e[39m";
		return;
	}

	std::string line;
	while (std::getline(lineHistoryFile, line))
		lineHistory.emplace_back(line);
}
