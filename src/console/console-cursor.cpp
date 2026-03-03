#include "console-priv.hpp"

// These save the palette and character of whatever tile the cursor moves on top of.
// fb2mv should always contain the full block ASCII character (219).
// Copilot mentioned a data race possibility here: this is disproved by the fact that 
// (as of right now) both toolchains only provide cooperative multithreading (manual yielding).
static u16 fbmvUnderCursor, fb2mvUnderCursor;

void consoleSaveTileUnderCursor(void) {
	fbmvUnderCursor = *consoleFontBgMapAtCursor();
	fb2mvUnderCursor = *consoleFontBg2MapAtCursor();
}

void consoleRestoreTileUnderCursor(void) {
	*consoleFontBgMapAtCursor() = fbmvUnderCursor;
	*consoleFontBg2MapAtCursor() = fb2mvUnderCursor;
}

void consoleDrawCursor(void) {
	// foreground: this is the character itself. we need to turn it black,
	// which would usually be a fontCurPal of TILE_PALETTE(0). however fbmvUnderCursor
	// is not being computed on the fly (like below), so 99% of the time it 
	// does not have those bits zeroed inside. (who uses black on black?)
	// so we need to zero-out those bits to make it black:
	*consoleFontBgMapAtCursor() = 0x0fff & fbmvUnderCursor;

	// background: just use bright white (15)
	*consoleFontBg2MapAtCursor() = TILE_PALETTE(15) | consoleComputeFontBg2MapValue(CONSOLE_CURSOR_CHAR);
}

void consoleSetCursorPos(const int x, const int y) {
	MyPrintConsole *c = getCurrentConsole();

	if (c->bg2Id != -1)
		consoleRestoreTileUnderCursor();

	c->cursorX = x;
	c->cursorY = y;

	if (c->bg2Id != -1) {
		consoleSaveTileUnderCursor();

		// original tile saved! now let's make the cursor visible with
		// white background and black foreground.
		consoleDrawCursor();
	}
}

void consoleSetCursorY(const int y) {
	consoleSetCursorPos(getCurrentConsole()->cursorX, y);
}

void consoleSetCursorX(const int x) {
	consoleSetCursorPos(x, getCurrentConsole()->cursorY);
}

int clamp(const int n, const int min, const int max) {
	if (n < min)
		return min;
	if (n > max)
		return max;
	return n;
}

void consoleMoveCursorX(const int dx) {
	const PrintConsole *const c = getCurrentConsole();
	const int newX = c->cursorX + dx;
	const int maxX = c->windowWidth - 1;
	consoleSetCursorX(clamp(newX, 0, maxX));
}

void consoleMoveCursorY(const int dy) {
	const PrintConsole *const c = getCurrentConsole();
	const int newY = c->cursorY + dy;
	const int maxY = c->windowHeight - 1;
	consoleSetCursorY(clamp(newY, 0, maxY));
}
