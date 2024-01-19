#include <nds.h>
#include <iostream>
#include <functional>
#include "Cursor.hpp"

struct KeyboardProcessor
{
	std::string line;
	unsigned cursorPos;
	bool flashState;
	u8 flashTimer, flashInterval = 20;
	bool newlineEntered;

	void ProcessKeyboard(void)
	{
	}

	void HandleIgnoredKey(const char cursorCharacter)
	{
		if (cursorPos == line.size() && flashTimer && flashState)
		{
			flashTimer = 0;
			flashState = false;
		}
		else if (++flashTimer >= flashInterval)
		{
			flashTimer = 0;
			((flashState = !flashState) ? (std::cout << cursorCharacter) : (std::cout << line[cursorPos])) << Cursor::moveLeftOne;
		}
	}

	void HandleEnter(void)
	{
		if (cursorPos == line.size())
			std::cout << ' ';
		else
			std::cout << line[cursorPos] << '\r';
		std::cout << '\n';
		newlineEntered = true;
	}

	void HandleBackspace(const char cursorCharacter)
	{
		if (!line.empty())
			line.erase(--cursorPos, 1);
		// space overwrites the cursor character, then backspace the space and the desired character
		std::cout << " \b\b";
		if (cursorCharacter)
			std::cout << cursorCharacter;
		std::cout << Cursor::moveLeftOne;
	}
};