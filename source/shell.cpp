#include <nds.h>
#include <string>
#include <iostream>
#include <vector>

#define FLASH_INTERVAL 20

auto ProcessLine(const std::string &line)
{
	std::cout << "Line processed: '" << line << "'\n";
	std::cout << "Line length: " << line.size() << '\n';
}

// Base string for an ANSI escape sequence
const auto ESC_SEQ_BASE = "\e[";

namespace Cursor
{
	const auto CHARACTER = '_';

	auto moveLeft(const int numColumns)
	{
		return ESC_SEQ_BASE + std::to_string(numColumns) + 'D';
	}

	auto moveRight(const int numColumns)
	{
		return ESC_SEQ_BASE + std::to_string(numColumns) + 'C';
	}

	const auto moveLeftOne = moveLeft(1);
	const auto moveRightOne = moveRight(1);
}

int main(void)
{
	// Video initialization - We want to use both screens
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_0_2D);
	vramSetBankA(VRAM_A_MAIN_BG);
	vramSetBankC(VRAM_C_SUB_BG);

	// Show the console on the top screen
	PrintConsole console;
	consoleInit(&console, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);

	// Initialize and show the keyboard on the bottom screen
	keyboardDemoInit();
	keyboardShow();

	// Create the shell prompt
	const auto prompt = std::string("> ") + Cursor::CHARACTER + Cursor::moveLeftOne;
	std::cout << prompt;

	// Variables
	signed char c;
	std::string line;
	unsigned cursorPos = 0;
	bool flashState = false;	  // Flag to track the flashing state
	unsigned char flashTimer = 0; // Timer to control the flashing interval

	while (1)
	{
		switch (c = keyboardUpdate())
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
			if (++flashTimer >= FLASH_INTERVAL)
			{
				flashTimer = 0;
				((flashState = !flashState) ? (std::cout << Cursor::CHARACTER) : (std::cout << line[cursorPos])) << Cursor::moveLeftOne;
			}
			break;

		// enter, newline, '\n', 10
		case DVK_ENTER:
			if (cursorPos == line.size())
				std::cout << ' ';
			else
				std::cout << line[cursorPos] << '\r';
			std::cout << '\n';
			ProcessLine(line);
			line.clear();
			cursorPos = 0;
			std::cout << prompt;
			break;

		// backspace, '\b', 8
		case DVK_BACKSPACE:
			if (!line.empty())
				line.erase(--cursorPos, 1);
			// space overwrites the cursor character, then backspace the space and the desired character
			std::cout << " \b\b" << Cursor::CHARACTER << Cursor::moveLeftOne;
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
				std::cout << Cursor::CHARACTER << Cursor::moveLeftOne;
			break;

		// any other valid character
		default:
			if (cursorPos == line.size())
				line += c;
			else
				line[cursorPos] = c;
			std::cout << c;
			if (++cursorPos == line.size())
				std::cout << Cursor::CHARACTER << Cursor::moveLeftOne;
		}

		// Stay synchronized with the screen's refresh rate (60Hz)
		// This isn't necessary, but probably better to not run this loop 100% of the time
		swiWaitForVBlank();

		// For testing
		assert(cursorPos >= 0 && cursorPos <= line.size());
	}
}
