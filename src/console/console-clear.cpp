#include "console-priv.hpp"

void consoleClearScreen(const char mode)
{
	MyPrintConsole *const c = getCurrentConsole();
	const u16 blank = ' ' + c->fontCharOffset - c->font.asciiOffset;
	const u16 blank_main = TILE_PALETTE(15) | blank;
	const u16 blank_bg2 = TILE_PALETTE(0) | blank;

	switch (mode)
	{
	case '[': // defensive: treat as mode 0 if somehow passed
	case '0': // from cursor to end of screen
		// Clear from cursor to end of line
		consoleClearLine('0');
		// Clear lines below
		for (int y = c->cursorY + 1; y < c->windowHeight; y++)
		{
			for (int x = 0; x < c->windowWidth; x++)
			{
				*consoleFontBgMapAt(x, y) = blank_main;
				if (c->bg2Id != -1)
					*consoleFontBg2MapAt(x, y) = blank_bg2;
			}
		}
		break;

	case '1': // from cursor to beginning of screen
		// Clear lines above
		for (int y = 0; y < c->cursorY; y++)
		{
			for (int x = 0; x < c->windowWidth; x++)
			{
				*consoleFontBgMapAt(x, y) = blank_main;
				if (c->bg2Id != -1)
					*consoleFontBg2MapAt(x, y) = blank_bg2;
			}
		}
		// Clear from start of line to cursor
		consoleClearLine('1');
		break;

	case '2': // whole screen
		for (int y = 0; y < c->windowHeight; y++)
		{
			for (int x = 0; x < c->windowWidth; x++)
			{
				*consoleFontBgMapAt(x, y) = blank_main;
				if (c->bg2Id != -1)
					*consoleFontBg2MapAt(x, y) = blank_bg2;
			}
		}
		c->cursorX = 0;
		c->cursorY = 0;
		break;
	}
}

void consoleClearLine(const char mode)
{
	const MyPrintConsole *const c = getCurrentConsole();
	int line = c->cursorY;

	// \e[K is same as \e[0K: from cursor to end of line
	// so use its parameters as the default.
	int start = c->cursorX;
	int end = c->windowWidth;

	if (mode == '1')
	{
		// start of line to cursor
		start = 0;
		end = c->cursorX;
	}
	else if (mode == '2')
	{
		// whole line
		start = 0;
		end = c->windowWidth;
	}

	// The character offset for ' ' in the font.
	const u16 blank = ' ' + c->fontCharOffset - c->font.asciiOffset;

	for (int i = start; i < end; ++i)
	{
		// Clear with default colors (fg: white, bg: black) regardless of current palette.
		*consoleFontBgMapAt(i, line) = TILE_PALETTE(15) | blank;
		if (c->bg2Id != -1)
			*consoleFontBg2MapAt(i, line) = TILE_PALETTE(0) | blank;
	}
}
