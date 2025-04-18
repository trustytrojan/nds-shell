#pragma once

#include <nds.h>

#include <iostream>
#include <string>

// Commandline prompt with a visible, movable cursor that can edit the line at
// any position.
class CliPrompt
{
	// The stream to print to.
	std::ostream &ostr;

	// State necessary for rendering a visible cursor
	u32 cursorPos{};
	bool flashState{};
	u8 flashTimer{};

	// Keypress state
	bool _newlineEntered{};
	bool _foldPressed{};

	inline void resetKeypressState() { _newlineEntered = _foldPressed = {}; }
	void flashCursor(const std::string &line);
	void handleBackspace(std::string &line);
	void handleEnter(const std::string &line);
	void handleLeft(const std::string &line);
	void handleRight(const std::string &line);

public:
	// The text to print before the cursor.
	std::string prompt;

	// The character to print as the cursor.
	char cursor;

	CliPrompt(
		const std::string &prompt = "> ",
		const char cursor = '_',
		std::ostream &ostr = std::cout);

	inline void resetProcessKeyboardState()
	{
		cursorPos = flashState = flashTimer = {};
	}

	// Processes the current state of the keyboard, and updates state as
	// necessary.
	void processKeyboard(std::string &line);

	// Starts processing the keyboard every frame until the Enter key is
	// pressed, then writes user input into `line`.
	void getLine(std::string &line);

	inline constexpr bool newlineEntered() const { return _newlineEntered; }
	inline constexpr bool foldPressed() const { return _foldPressed; }
};
