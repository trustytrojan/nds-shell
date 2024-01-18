#include <nds.h>
#include <string>
#include <iostream>

void ProcessLine(const std::string &line)
{
	std::cout << "Line processed: '" << line << "'\n";
	std::cout << "Line length: " << line.size() << '\n';
}

// Base string for an ANSI escape sequence
const auto ESC_SEQ_BASE = "\e[";

namespace Cursor
{
	const auto CHARACTER = '_';

	const auto moveLeft(const int numColumns = 1)
	{
		return ESC_SEQ_BASE + std::to_string(numColumns) + 'D';
	}

	const auto moveRight(const int numColumns = 1)
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
	unsigned char c;
	std::string line;
	unsigned cursorPos = 0;

	while (1)
	{
		switch (c = keyboardUpdate())
		{
		// obviously ignore null bytes, and 255 is returned when nothing is pressed
		case 0:
		case 255:
			break;

		// enter, newline, '\n', 10
		case DVK_ENTER:
			std::cout << ' ' << '\n';
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
			std::cout << ' ' << c << c << Cursor::CHARACTER << Cursor::moveLeftOne;
			break;

		// left arrow
		case DVK_LEFT:
			if (cursorPos > 0)
				--cursorPos;
			std::cout << Cursor::moveLeftOne;
			break;

		// right arrow
		case DVK_RIGHT:
			if (cursorPos < line.size())
				++cursorPos;
			std::cout << Cursor::moveRightOne;
			break;

		// any other valid character
		default:
			line += c;
			std::cout << c << Cursor::CHARACTER << Cursor::moveLeftOne;
		}

		// Stay synchronized with the screen's refresh rate (60Hz)
		// This isn't necessary, but probably better to not run this loop 100% of the time
		swiWaitForVBlank();
	}
}
