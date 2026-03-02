#include "console-priv.hpp"

u16 *consoleFontBgMapAt(const int x, const int y) {
	const PrintConsole *const c = getCurrentConsole();
	const int xOffset = x + c->windowX;
	const int yOffset = (y + c->windowY) * c->consoleWidth;
	return c->fontBgMap + xOffset + yOffset;
}

u16 *consoleFontBg2MapAt(const int x, const int y) {
	const MyPrintConsole *const c = getCurrentConsole();
	const int xOffset = x + c->windowX;
	const int yOffset = (y + c->windowY) * c->consoleWidth;
	return c->fontBg2Map + xOffset + yOffset;
}

u16 *consoleFontBgMapAtCursor(void) {
	const PrintConsole *const c = getCurrentConsole();
	return consoleFontBgMapAt(c->cursorX, c->cursorY);
}

u16 *consoleFontBg2MapAtCursor(void) {
	const PrintConsole *const c = getCurrentConsole();
	return consoleFontBg2MapAt(c->cursorX, c->cursorY);
}

u16 consoleComputeFontBgMapValue(const char ch) {
	const PrintConsole *const c = getCurrentConsole();
	return c->fontCurPal | (u16)(ch + c->fontCharOffset - c->font.asciiOffset);
}

u16 consoleComputeFontBg2MapValue(const char ch) {
	const MyPrintConsole *const c = getCurrentConsole();
	return c->fontCurPal2 | (u16)(ch + c->fontCharOffset - c->font.asciiOffset);
}

static void newRow() {
	MyPrintConsole *const c = getCurrentConsole();

	if (c->bg2Id != -1)
		consoleRestoreTileUnderCursor();

	c->cursorY++;

	if (c->cursorY >= c->windowHeight) {
		int rowCount;
		int colCount;

		c->cursorY--;

		for (rowCount = 0; rowCount < c->windowHeight - 1; rowCount++)
			for (colCount = 0; colCount < c->windowWidth; colCount++) {
				*consoleFontBgMapAt(colCount, rowCount) = *consoleFontBgMapAt(colCount, rowCount + 1);
				if (c->bg2Id != -1)
					*consoleFontBg2MapAt(colCount, rowCount) = *consoleFontBg2MapAt(colCount, rowCount + 1);
			}

		for (colCount = 0; colCount < c->windowWidth; colCount++) {
			*consoleFontBgMapAt(colCount, rowCount) = consoleComputeFontBgMapValue(' ');
			if (c->bg2Id != -1)
				*consoleFontBg2MapAt(colCount, rowCount) = consoleComputeFontBg2MapValue(' ');
		}
	}

	if (c->bg2Id != -1)
		consoleSaveTileUnderCursor();
}

void consoleCommitChar(const char ch) {
	MyPrintConsole *const c = getCurrentConsole();

	*consoleFontBgMapAtCursor() = consoleComputeFontBgMapValue(ch); // fg

	if (c->bg2Id != -1)
		*consoleFontBg2MapAtCursor() = consoleComputeFontBg2MapValue(219); // bg

	++c->cursorX;

	if (c->bg2Id != -1) {
		consoleSaveTileUnderCursor();
		consoleDrawCursor();
	}
}

// could have a better name, since we aren't always printing a character
void myConsolePrintChar(const char ch) {
	if (!ch)
		return;

	MyPrintConsole *const c = getCurrentConsole();

	if (!c->fontBgMap)
		return;

	if (ch + c->fontCharOffset - c->font.asciiOffset > c->font.numChars)
		// this prevents weird stuff showing up when printing!
		return;

	if (c->PrintChar && c->PrintChar(c, ch))
		return;

	if (c->cursorX >= c->windowWidth) {
		if (c->bg2Id == -1)
			c->cursorX = 0;
		else
			consoleSetCursorX(0);
		newRow();
	}

	switch (ch) {
	case '\a':
		// bell character: TODO: add a callback for applications to respond to this. e.g. playing a bell sound
		break;

	case '\b':
		/*
			the old code here actually moved the cursor back one (and up one row
			if needed) and then "erased" the character by writing a space to fontBgMap.

			in a linux terminal emulator with the termios attr ICANON off,
			backspace does NOT move the cursor. if the ECHO termios attr is on
			it prints the \b character (visualized as ^?).

			with ICANON and ECHO on, backspace does NOT wrap the cursor around to the
			last row.

			what dkp was trying to do here is emulate "canonical mode"
			(ICANON termios attr), which tells the terminal to line-buffer
			input from the keyboard before sending it to stdin. but it ALSO
			does what WE (the console) were doing here before: erasing the current character
			and moving the cursor back. THIS SHOULD BE KEYBOARD.C's JOB!

			SOLUTION:
			keyboard.c should have both an "echo" and "line buffer" option.
			when keyboard.c's "line buffering" is on, **it should interface with the
			currently selected console** to erase the character at the cursor and then move
			the cursor back. when "echo" is on, keyboard.c should send what it gets to the
			console to be ***immediately*** rendered, with non-visual characters visualized.
			for example, left arrow becomes ^[[D.

			the responsibilities should NOT be mixed together.
		*/
		break;

	case '\t':
		if (c->bg2Id == -1)
			c->cursorX += c->tabSize - ((c->cursorX) % (c->tabSize));
		else
			consoleMoveCursorX(c->tabSize - ((c->cursorX) % (c->tabSize)));
		break;

	case '\n':
		newRow();
		// also return to first column by falling through to the '\r' case:

	case '\r':
		if (c->bg2Id == -1)
			c->cursorX = 0;
		else
			consoleSetCursorX(0);
		break;

	default:
		*consoleFontBgMapAtCursor() = consoleComputeFontBgMapValue(ch); // fg

		if (c->bg2Id != -1)
			*consoleFontBg2MapAtCursor() = consoleComputeFontBg2MapValue(219); // bg

		++c->cursorX;

		if (c->bg2Id != -1) {
			consoleSaveTileUnderCursor();
			consoleDrawCursor();
		}
	}
}
