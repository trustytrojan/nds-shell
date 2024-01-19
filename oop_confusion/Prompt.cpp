#include "Prompt.hpp"

std::string Prompt::GetLine(void) const
{
	std::string line;
	u32 cursorPos = 0;
	bool flashState = false;
	u8 flashTimer = 0;
	bool newlineEntered = false;

	std::cout << basePrompt;

	while (!newlineEntered)
	{
		ProcessKeyboard(line, cursorPos, flashState, flashTimer, newlineEntered);
		swiWaitForVBlank();
	}

	return line;
}

void Prompt::ProcessKeyboard(std::string &line, u32 &cursorPos, bool &flashState, u8 &flashTimer, bool &newlineEntered) const
{
	if (newlineEntered)
		return;
	const s8 c = keyboardUpdate();
	switch (c)
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
		// if cursorPos isn't the end of line, flash the current character between '_' and itself every (1 / FLASH_INTERVAL) seconds
		if (cursorPos == line.size())
		{
			flashTimer = 0;
			flashState = false;
			break;
		}
		if (++flashTimer >= flashInterval)
		{
			flashTimer = 0;
			((flashState = !flashState) ? (std::cout << cursorCharacter) : (std::cout << line[cursorPos])) << Cursor::moveLeftOne;
		}
		break;

	// enter, newline, '\n', 10
	case DVK_ENTER:
		if (cursorPos == line.size())
			std::cout << ' ';
		else
			std::cout << line[cursorPos] << '\r';
		std::cout << '\n';
		newlineEntered = true;
		break;

	// backspace, '\b', 8
	case DVK_BACKSPACE:
		if (!line.empty())
			line.erase(--cursorPos, 1);
		// space overwrites the cursor character, then backspace the space and the desired character
		std::cout << " \b\b" << cursorCharacter << Cursor::moveLeftOne;
		break;

	// left arrow
	case DVK_LEFT:
		if (cursorPos == 0)
			break;
		if (cursorPos == line.size())
			std::cout << " \b";
		if (flashState)
			std::cout << line[cursorPos] << Cursor::moveLeftOne;
		std::cout << Cursor::moveLeftOne;
		--cursorPos;
		break;

	// right arrow
	case DVK_RIGHT:
		if (cursorPos == line.size())
			break;
		std::cout << line[cursorPos];
		if (++cursorPos == line.size())
			std::cout << cursorCharacter << Cursor::moveLeftOne;
		break;

	// any other valid character
	default:
		if (cursorPos == line.size())
			line += c;
		else
			line[cursorPos] = c;
		std::cout << c;
		if (++cursorPos == line.size())
			std::cout << cursorCharacter << Cursor::moveLeftOne;
	}
}
