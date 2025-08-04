#include "CliPrompt.hpp"
#include "EscapeSequences.hpp"

#include <fstream>
#include <iostream>

using namespace EscapeSequences;

void CliPrompt::printFullPrompt(bool withInput)
{
	*ostr << prompt;
	if (withInput)
		*ostr << input;
	*ostr << cursor << Cursor::moveLeftOne;
}

void CliPrompt::processUntilEnterPressed()
{
	prepareForNextLine();
	printFullPrompt(false);
	do
	{
		processKeyboard();
		swiWaitForVBlank();
	} while (pmMainLoop() && !_enterPressed);
}

void CliPrompt::prepareForNextLine()
{
	cursorPos = flashState = flashTimer = {};
	lineHistoryItr = lineHistory.cend();
	savedInput.clear();
	input.clear();
}

void CliPrompt::flashCursor()
{
	/**
	 * While `cursorPos != line.size()`, the character pointed to by the cursor
	 * will flash between `line[cursorPos]` and `this->cursor`.
	 */
	static const auto FLASH_INTERVAL = 10;

	if (cursorPos == input.size())
	{
		flashTimer = 0;
		flashState = false;
	}
	else if (++flashTimer >= FLASH_INTERVAL)
	{
		flashTimer = 0;
		((flashState = !flashState) ? (*ostr << cursor)
									: (*ostr << input[cursorPos]))
			<< Cursor::moveLeftOne;
	}
}

void CliPrompt::handleBackspace()
{
	if (input.empty() || cursorPos == 0)
		return;
	input.erase(--cursorPos, 1);
	if (cursorPos == input.size())
		*ostr << " \b\b" << cursor << Cursor::moveLeftOne;
	else
		*ostr << "\b\e[0K" << input.c_str() + cursorPos
			  << Cursor::move(
					 Cursor::MoveDirection::LEFT, input.size() - cursorPos);
}

void CliPrompt::handleEnter()
{
	if (cursorPos == input.size())
		*ostr << ' ';
	else
		*ostr << input[cursorPos] << '\r';

	*ostr << '\n';
	_enterPressed = true;

	// don't push empty lines!
	if (input.size())
		lineHistory.emplace_back(input);
}

void CliPrompt::handleLeft()
{
	if (input.empty() || cursorPos == 0)
		return;
	if (cursorPos == input.size())
		*ostr << " \b";
	if (flashState)
		*ostr << input[cursorPos] << Cursor::moveLeftOne;
	*ostr << Cursor::moveLeftOne;
	--cursorPos;
}

void CliPrompt::handleRight()
{
	if (input.empty() || cursorPos == input.size())
		return;
	*ostr << input[cursorPos];
	if (++cursorPos == input.size())
		*ostr << cursor << Cursor::moveLeftOne;
}

void CliPrompt::handleUp()
{
	if (lineHistory.empty() || lineHistoryItr - 1 < lineHistory.cbegin())
		// obviously do nothing here
		return;

	if (lineHistoryItr == lineHistory.cend())
		// save the "new" line for when the user goes all the way down again
		savedInput = input;

	// go up one, reset cursorPos, reprint everything
	input = *--lineHistoryItr;
	cursorPos = input.size();
	*ostr << "\r\e[2K" << prompt << input << cursor << Cursor::moveLeftOne;
}

void CliPrompt::handleDown()
{
	if (lineHistory.empty() || lineHistoryItr + 1 > lineHistory.cend())
		// obviously do nothing here
		return;

	// go down one
	++lineHistoryItr;

	if (lineHistoryItr == lineHistory.cend())
	{
		// we're at the "new" line, restore it
		input = savedInput;
		cursorPos = input.size();
		*ostr << "\r\e[2K" << prompt << input << cursor << Cursor::moveLeftOne;
		return;
	}

	// go down one, reset cursorPos, reprint everything
	input = *lineHistoryItr;
	cursorPos = input.size();
	*ostr << "\r\e[2K" << prompt << input << cursor << Cursor::moveLeftOne;
}

void CliPrompt::processKeyboard()
{
	resetKeypressState();

	scanKeys();
	const auto keys = keysDownRepeat();
	if (keys & (KEY_A | KEY_START))
		handleEnter();
	if (keys & KEY_B)
		handleBackspace();
	if (keys & KEY_LEFT)
		handleLeft();
	if (keys & KEY_RIGHT)
		handleRight();
	if (keys & KEY_UP)
		handleUp();
	if (keys & KEY_DOWN)
		handleDown();

	// this needs to be cast to a char for printing
	// keep it signed because of the negative value enums to check against!
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
		flashCursor();
		break;

	case DVK_FOLD:
		_foldPressed = true;
		break;

	case DVK_ENTER:
		if (keys & KEY_A)
			break;
		handleEnter();
		break;

	case DVK_BACKSPACE:
		if (keys & KEY_B)
			break;
		handleBackspace();
		break;

	case DVK_LEFT:
		if (keys & KEY_LEFT)
			break;
		handleLeft();
		break;

	case DVK_RIGHT:
		if (keys & KEY_RIGHT)
			break;
		handleRight();
		break;

	case DVK_UP:
		if (keys & KEY_UP)
			break;
		handleUp();
		break;

	case DVK_DOWN:
		if (keys & KEY_DOWN)
			break;
		handleDown();
		break;

	default:
		if (cursorPos == input.size())
		{
			input += c;
			*ostr << c;
		}
		else
		{
			input.insert(input.begin() + cursorPos, c);
			*ostr << input.c_str() + cursorPos
				  << Cursor::move(
						 Cursor::MoveDirection::LEFT,
						 input.size() - cursorPos - 1);
		}
		if (++cursorPos == input.size())
			*ostr << cursor << Cursor::moveLeftOne;
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
