#pragma once

#include <nds/arm9/console.h>
#include <nds/ndstypes.h>

// BlocksDS provides TILE_PALETTE, but devkitPro does not
#ifndef TILE_PALETTE
	#define TILE_PALETTE(n) ((n) << 12)
#endif

#ifdef __BLOCKSDS__
	#define CONSOLE_CURSOR_CHAR '_'
#else
	#define CONSOLE_CURSOR_CHAR 219
#endif

struct MyPrintConsole : PrintConsole
{
	int bg2Id; // ID of bg used for ANSI background colors!
	u16 *fontBg2Map, *fontBg2Gfx; // map & gfx for the 2nd bg
	u16 fontCurPal2; // palette for background colors

	// Internal buffer for potential ANSI escape sequences, as all the characters may not be available in one con_write() call.
	char escBuf[32];
	int escBufLen;
};

constexpr MyPrintConsole *getCurrentConsole()
{
	const auto currentConsole = (MyPrintConsole *)consoleSelect(NULL);
	consoleSelect(currentConsole);
	return currentConsole;
}

// console-clear.cpp
void consoleClearScreen(char mode);
void consoleClearLine(char mode);

// console-print.cpp
u16 *consoleFontBgMapAt(int x, int y);
u16 *consoleFontBg2MapAt(int x, int y);
u16 *consoleFontBgMapAtCursor(void);
u16 *consoleFontBg2MapAtCursor(void);
u16 consoleComputeFontBgMapValue(char);
u16 consoleComputeFontBg2MapValue(char);
void myConsolePrintChar(char);

// console-cursor.cpp
void consoleSaveTileUnderCursor(void);
void consoleRestoreTileUnderCursor(void);
void consoleMoveCursorX(int dx); // clamps cursorX to windowWidth
void consoleMoveCursorY(int dy); // clamps cursorY to windowHeight
void consoleSetCursorX(int x);
void consoleSetCursorY(int y);
void consoleSetCursorPos(int x, int y);
void consoleDrawCursor(void);

// console-esc.cpp
void consoleUpdateEscapeSequence(char c);
int consoleParseEscapeSequence(const char *ptr, int len);
