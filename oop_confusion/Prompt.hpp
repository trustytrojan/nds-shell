#ifndef __PROMPT__
#define __PROMPT__

#include <nds.h>
#include <iostream>
#include <functional>
#include "Cursor.hpp"

/**
 * Contains state to customize and start a command-line prompt for getting user input.
 */
struct Prompt
{
	/**
	 * The text to print before the cursor. For example `basePrompt = "> "`
	 * and `cursorCharacter = '_'` would result in the prompt looking like
	 * `> _`.
	 */
	std::string basePrompt = "> ";

	/**
	 * The character to use as the text cursor.
	 */
	char cursorCharacter = '_';

	/**
	 * If the cursor isn't at the end of line, the character at the cursor
	 * will flash between the cursor character and the character in the line
	 * string every `1 / (60 / flashInterval)` seconds.
	 */
	s8 flashInterval = 20;

	/**
	 * Starts the prompt and returns the line when Enter is pressed.
	 * \returns The line entered.
	 */
	std::string GetLine(void) const;

	/**
	 * Process one keypress, updating state as necessary. Designed to be called in a
	 * loop synchronized with the DS screen's refresh rate. See the implementation
	 * of `Prompt::GetLine`.
	 */
	void ProcessKeyboard(std::string &line, u32 &cursorPos, bool &flashState, u8 &flashTimer, bool &newlineEntered) const;
};

#endif