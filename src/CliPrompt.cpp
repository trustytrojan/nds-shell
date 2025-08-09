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
	//*ostr << cursor << Cursor::moveLeftOne;
}

void CliPrompt::prepareForNextLine()
{
	cursorPos = flashState = {};
	lineHistoryItr = lineHistory.cend();
	savedInput.clear();
	input.clear();
}

// void flashCursorTickTask(TickTask *t)
// {
// 	auto &mtt = *(MyTickTask *)t;
// 	mtt.flashState = !mtt.flashState;
// }

// void CliPrompt::flashCursor()
// {
// 	/**
// 	 * While `cursorPos != line.size()`, the character pointed to by the cursor
// 	 * will flash between `line[cursorPos]` and `this->cursor`.
// 	 */
// 	*ostr << (flashState ? cursor : input[cursorPos]) << Cursor::moveLeftOne;
// }

void CliPrompt::handleBackspace()
{
	if (input.empty() || cursorPos == 0)
		return;
	input.erase(--cursorPos, 1);
	if (cursorPos == input.size())
		// *ostr << " \b\b" << cursor << Cursor::moveLeftOne;
		// *ostr << "\b \b";
		*ostr << Cursor::moveLeftOne << ' ' << Cursor::moveLeftOne;
	else
		*ostr << Cursor::moveLeftOne << "\e[0K" << input.c_str() + cursorPos
			  << Cursor::move(Cursor::MoveDirection::LEFT, input.size() - cursorPos);
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
	{ // above to move off last char
	  // remove static cursor
	  // *ostr << " \b";

		// if (!mttRunning)
		// {
		// 	tickTaskStart(&mtt, flashCursorTickTask, ticksFromHz(6), ticksFromHz(6));
		// 	mttRunning = true;
		// }
	}

	// if (flashState)
	// prevent leaving behind cursors over the input
	// *ostr << input[cursorPos] << Cursor::moveLeftOne;

	*ostr << Cursor::moveLeftOne;
	--cursorPos;
}

void CliPrompt::handleRight()
{
	if (input.empty() || cursorPos == input.size())
		return;

	// *ostr << input[cursorPos];
	*ostr << Cursor::moveRightOne;

	if (++cursorPos == input.size())
	{
		// at end of line, reprint cursor
		// *ostr << cursor << Cursor::moveLeftOne;

		// if (mttRunning)
		// {
		// 	tickTaskStop(&mtt);
		// 	mttRunning = false;
		// }
	}
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
	*ostr << "\r\e[2K" << prompt << input; //<< cursor << Cursor::moveLeftOne;
}

void CliPrompt::handleDown()
{
	if (lineHistory.empty() || lineHistoryItr == lineHistory.cend())
		// obviously do nothing here
		return;

	// go down one
	++lineHistoryItr;

	if (lineHistoryItr == lineHistory.cend())
	{
		// we're at the "new" line, restore it
		input = savedInput;
		cursorPos = input.size();
		*ostr << "\r\e[2K" << prompt << input; // << cursor << Cursor::moveLeftOne;
		return;
	}

	// reset cursorPos, reprint everything
	input = *lineHistoryItr;
	cursorPos = input.size();
	*ostr << "\r\e[2K" << prompt << input; // << cursor << Cursor::moveLeftOne;
}

bool CliPrompt::processKeypad()
{
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
	case DVK_TAB: // ignore tabs for now, use them for autocomplete later
	case DVK_CTRL:
	case DVK_ALT:
	case DVK_CAPS:
	case DVK_MENU:
	case DVK_SHIFT:
		// flashCursor();
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
			*ostr << input.c_str() + cursorPos
				  << Cursor::move(Cursor::MoveDirection::LEFT, input.size() - cursorPos - 1);
		}
		++cursorPos;
		// if (++cursorPos == input.size())
		// *ostr << cursor << Cursor::moveLeftOne;
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
