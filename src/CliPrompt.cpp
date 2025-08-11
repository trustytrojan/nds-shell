#include "CliPrompt.hpp"
#include "Shell.hpp"

#include <fstream>
#include <iostream>

void CliPrompt::printFullPrompt(bool withInput)
{
	*ostr << prompt;
	if (withInput)
		*ostr << input;
}

void CliPrompt::prepareForNextLine()
{
	cursorPos = flashState = {};
	lineHistoryItr = lineHistory.cend();
	savedInput.clear();
	input.clear();
}

const auto MOVE_LEFT = "\e[D";
const auto MOVE_RIGHT = "\e[C";

void CliPrompt::handleBackspace()
{
	if (input.empty() || cursorPos == 0)
		return;
	input.erase(--cursorPos, 1);
	if (cursorPos == input.size())
		*ostr << MOVE_LEFT << ' ' << MOVE_LEFT;
	else
		// move left, clear to end of line (\e[0K), reprint input from cursor onwards, move back to cursor position
		*ostr << MOVE_LEFT << "\e[0K" << (input.c_str() + cursorPos) << "\e[" << (input.size() - cursorPos) << 'D';
}

void CliPrompt::handleEnter()
{
	*ostr << '\n';
	_enterPressed = true;

	// don't save empty lines!
	if (input.size())
		lineHistory.emplace_back(input);
}

void CliPrompt::handleLeft()
{
	if (input.empty() || cursorPos == 0)
		return;
	--cursorPos;
	*ostr << MOVE_LEFT;
}

void CliPrompt::handleRight()
{
	if (input.empty() || cursorPos == input.size())
		return;
	++cursorPos;
	*ostr << MOVE_RIGHT;
}

void CliPrompt::handleUp()
{
	if (lineHistory.empty() || lineHistoryItr == lineHistory.cbegin())
		// obviously do nothing here
		return;

	if (lineHistoryItr == lineHistory.cend())
		// save the "new" line for when the user goes all the way down again
		savedInput = input;

	// go up one, reset cursorPos, reprint everything
	input = *--lineHistoryItr;
	cursorPos = input.size();
	*ostr << "\r\e[2K" << prompt << input;
}

void CliPrompt::handleDown()
{
	if (lineHistory.empty() || lineHistoryItr == lineHistory.cend())
		// obviously do nothing here
		return;

	// go down one
	++lineHistoryItr;

	if (lineHistoryItr == lineHistory.cend())
		// we're at the "new" line, restore it
		input = savedInput;
	else
		input = *lineHistoryItr;

	// reset cursorPos, reprint everything
	cursorPos = input.size();
	*ostr << "\r\e[2K" << prompt << input;
}

bool CliPrompt::processKeypad()
{
#ifndef NDSH_THREADING
	scanKeys();
#endif
	const auto keys = keysDown();

	if (keys & (KEY_A | KEY_START))
		handleEnter();
	else if (keys & KEY_B)
		handleBackspace();
	else if (keys & KEY_LEFT)
		handleLeft();
	else if (keys & KEY_RIGHT)
		handleRight();
	else if (keys & KEY_UP)
		handleUp();
	else if (keys & KEY_DOWN)
		handleDown();
	else
		return false;

	return true;
}

void CliPrompt::processKeyboard()
{
	// this needs to be cast to a char for printing
	// keep it signed because of the negative value enums to check against!
	switch (const s8 c = keyboardUpdate())
	{
	case 0: // just in case
	case NOKEY:
	case DVK_TAB: // TODO: use tabs for autocomplete
	case DVK_CTRL:
	case DVK_ALT:
	case DVK_CAPS:
	case DVK_MENU:
	case DVK_SHIFT:
		break;

	case DVK_FOLD:
		_foldPressed = true;
		break;

	case DVK_ENTER:
		handleEnter();
		break;

	case DVK_BACKSPACE:
		handleBackspace();
		break;

	case DVK_LEFT:
		handleLeft();
		break;

	case DVK_RIGHT:
		handleRight();
		break;

	case DVK_UP:
		handleUp();
		break;

	case DVK_DOWN:
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
			// move back (size - pos - 1), because we want the cursor to have "advanced" after inserting the character
			*ostr << (input.c_str() + cursorPos) << "\e[" << (input.size() - cursorPos - 1) << 'D';
		}
		++cursorPos;
	}
}

void CliPrompt::update()
{
	resetKeypressState();

	// only one input device can fire events at a time to reduce multithreaded issues
	if (processKeypad()) // handled an event:
		return;

	processKeyboard();
}

void CliPrompt::setLineHistory(const std::string &filename)
{
	lineHistory.clear();

	if (!Shell::fsInitialized())
		return;

	std::ifstream lineHistoryFile{filename};
	if (!lineHistoryFile)
	{
		std::cerr << "\e[91mfailed to load line history\n\e[39m";
		return;
	}

	std::string line;
	while (std::getline(lineHistoryFile, line))
		lineHistory.emplace_back(line);
}
