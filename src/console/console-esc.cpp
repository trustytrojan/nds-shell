#include "console-priv.hpp"
#include <ctype.h>
#include <stdio.h>

#ifdef __BLOCKSDS__
	#define siscanf sscanf
#endif

static void updateColorBright(const int param, int *const color, int *const bgcolor, int *const bright) {
	if (param == 0) { // Reset
		*color = 15;  // fg: bright white
		*bgcolor = 0; // bg: black
		*bright = 0;
	} else if (param == 1) { // Bright/bold
		*bright = 1;
	} else if (param >= 30 && param <= 37) { // fg color
		*color = param - 30;
	} else if (param >= 40 && param <= 47) { // bg color
		*bgcolor = param - 40;
	} else if (param >= 90 && param <= 97) { // bright fg color
		*color = param - 90 + 8;
	} else if (param >= 100 && param <= 107) { // bright bg color
		*bgcolor = param - 100 + 8;
	} else if (param == 39 || param == 49) { // Default color
		*color = 15;						 // fg: bright white
		*bgcolor = 0;						 // bg: black
	}
}

static void consoleParseColor(const char *const escapeseq) {
	MyPrintConsole *const c = getCurrentConsole();

	if (!c)
		return;

	// Special case: \x1b[m resets attributes
	if (*escapeseq == 'm') {
		c->fontCurPal = 15; // Default color (bright white)
		c->fontCurPal2 = 0;	 // black bg
		return;
	}

	// ignore truecolor rgb sequences in the form `ESC[38;2;{r};{g};{b}m`
	if (siscanf(escapeseq, "38;2;%*d;%*d;%*dm") > 0)
		return;

	unsigned id;
	// 256 color format sequence: `ESC[38;5;{ID}m`
	// support it, but just use 0-15, ignore everything else.
	if (siscanf(escapeseq, "38;5;%um", &id) > 0) {
		if (id <= 15)
			c->fontCurPal = id;
		return;
	}

	// -1 color means unchanged
	int color = -1, bgcolor = -1, bright = 0, items_matched, chars_consumed, param;
	const char *p = escapeseq;

	// %n does not match anything, it stores the number of characters
	// consumed thus far into the next pointer! use this to advance `p`.

	// start consuming arguments, delimited with ';'
	while (true) {
		// Try to parse a parameter followed by a semicolon
		chars_consumed = 0;
		items_matched = siscanf(p, "%d;%n", &param, &chars_consumed);
		if (items_matched > 0 && chars_consumed > 0) {
			updateColorBright(param, &color, &bgcolor, &bright);
			p += chars_consumed;
			continue;
		}

		// If that failed, try to parse the final parameter ending in 'm'
		items_matched = siscanf(p, "%dm", &param);
		if (items_matched > 0)
			updateColorBright(param, &color, &bgcolor, &bright);

		// End of sequence
		break;
	}

	// handle cases when only bold (1) modifier is used. this only affects foreground.
	// this should simply turn the current color into its bright variant.
	int final_color = -1;
	if (color != -1) {
		final_color = color;
		if (bright && final_color < 8)
			final_color += 8;
	} else if (bright) {
		// if only intensity is set, brighten the current color
		int current_color = c->fontCurPal;
		if (current_color < 8)
			final_color = current_color + 8;
		else
			final_color = current_color;
	}

	// final_color and bgcolor are 4-bit palette indices (0-15).

#ifdef __BLOCKSDS__
	// dkp's consoleInit() assigns fontCurPal (15 << 12), and not just 15 like BlocksDS
	// so let's just treat all fontCurPal assignments the same, depending on the toolchain
	#undef TILE_PALETTE
	#define TILE_PALETTE(x) (x)
#endif

	if (final_color != -1)
		c->fontCurPal = TILE_PALETTE(final_color);
	if (bgcolor != -1)
		c->fontCurPal2 = TILE_PALETTE(bgcolor);
}

static void consoleParseCsiSequence(void) {
	MyPrintConsole *const c = getCurrentConsole();
	const char *const seq = c->escBuf + 2; // Skip "\e["

	// The last character decides the function of the sequence
	const char command = c->escBuf[c->escBufLen - 1];

	switch (command) {
	// Private modes - commonly end in 'h' or 'l'. We don't implement any (yet), so just consume them.
	case 'h':
	case 'l':
		break;

	// Move cursor up
	case 'A': {
		int dy = 1;
		siscanf(seq, "%dA", &dy);
		consoleMoveCursorY(-dy);
		break;
	}

	// Move cursor down
	case 'B': {
		int dy = 1;
		siscanf(seq, "%dB", &dy);
		consoleMoveCursorY(dy);
		break;
	}

	// Move cursor right
	case 'C': {
		int dx = 1;
		siscanf(seq, "%dC", &dx);
		consoleMoveCursorX(dx);
		break;
	}

	// Move cursor left
	case 'D': {
		int dx = 1;
		siscanf(seq, "%dD", &dx);
		consoleMoveCursorX(-dx);
		break;
	}

	// Set cursor position
	case 'H':
	case 'f': {
		int x, y;
		if (siscanf(seq, "%d;%d", &y, &x) == 2)
			consoleSetCursorPos(x, y);
		else
			consoleSetCursorPos(0, 0);
		break;
	}

	// Screen clear
	case 'J': {
		int mode = 0;
		siscanf(seq, "%d", &mode);
		consoleClearScreen('0' + mode);
		break;
	}

	// Line clear
	case 'K': {
		int mode = 0;
		siscanf(seq, "%d", &mode);
		consoleClearLine('0' + mode);
		break;
	}

	// Save cursor position
	case 's':
		c->prevCursorX = c->cursorX;
		c->prevCursorY = c->cursorY;
		break;

	// Load cursor position
	case 'u':
		consoleSetCursorPos(c->prevCursorX, c->prevCursorY);
		break;

	// Color/style modes
	case 'm':
		consoleParseColor(seq);
		break;
	}
}

static void dumpAndResetEscBuf(void) {
	MyPrintConsole *const c = getCurrentConsole();
	for (int i = 0; i < c->escBufLen; i++)
		myConsolePrintChar(c->escBuf[i]);
	c->escBufLen = 0;
}

void consoleUpdateEscapeSequence(const char ch) {
	MyPrintConsole *const c = getCurrentConsole();

	if (!c)
		return;

	if (ch == '\e') {
		// We're most likely supposed to start buffering a new sequence.
		dumpAndResetEscBuf();
	}

	// Append to buffer
	c->escBuf[c->escBufLen++] = ch;
	c->escBuf[c->escBufLen] = '\0'; // just to be safe

	// Do we only have an escape in the buffer?
	if (c->escBufLen == 1 && c->escBuf[0] == '\e')
		// Don't continue, we don't want to dump the buffer now.
		return;

	// Do we have a CSI at the beginning of the buffer?
	if (c->escBuf[0] == '\e' && c->escBuf[1] == '[') {
		// Did we just form a full sequence? The sequences we parse all end in alphabetical characters.
		if (isalpha(ch)) {
			consoleParseCsiSequence();
			c->escBufLen = 0;
		}
	} else {
		dumpAndResetEscBuf();
	}

	if (c->escBufLen >= sizeof(c->escBuf) - 1)
		// Escape sequences shouldn't be this long.
		dumpAndResetEscBuf();
}
