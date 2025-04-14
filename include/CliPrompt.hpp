#pragma once

#include <nds.h>

#include <iostream>
#include <string>

// Commandline prompt with a visible, movable cursor that can edit the line at
// any position.
class CliPrompt
{
	// State necessary for ProcessKeyboard
	u32 cursorPos{};
	bool flashState{};
	u8 flashTimer{};
	bool newlineEntered{};

public:
	// The text to print before the cursor.
	const std::string prompt;

	// The character to print as the cursor.
	const char cursor;

	// The stream to print to.
	std::ostream &ostr;

	CliPrompt(
		const std::string &prompt = "> ",
		const char cursor = '_',
		std::ostream &ostr = std::cout);

	inline void resetProcessKeyboardState()
	{
		cursorPos = flashState = flashTimer = newlineEntered = {};
	}

	// Processes the current state of the keyboard, and updates state as
	// necessary.
	void ProcessKeyboard(std::string &line);

	// Starts processing the keyboard every frame until the Enter key is
	// pressed, then writes user input into `line`.
	void GetLine(std::string &line);

	inline constexpr bool newlineWasEntered() const { return newlineEntered; }
};
