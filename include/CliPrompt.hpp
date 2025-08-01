#pragma once

#include <nds.h>

#include <iostream>
#include <string>
#include <vector>

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

	std::vector<std::string> lineHistory;
	std::vector<std::string>::const_iterator lineHistoryItr;
	std::string lineToBeAdded;

	inline void resetKeypressState() { _newlineEntered = _foldPressed = {}; }
	void flashCursor(const std::string &line);
	void handleBackspace(std::string &line);
	void handleEnter(const std::string &line);
	void handleLeft(const std::string &line);
	void handleRight(const std::string &line);
	void handleUp(std::string &line);
	void handleDown(std::string &line);

public:
	// The text to print before the cursor.
	std::string prompt;

	// The character to print as the cursor.
	char cursor;

	CliPrompt(
		const std::string &prompt = "> ",
		const char cursor = '_',
		std::ostream &ostr = std::cout);

	void resetProcessKeyboardState();

	// Processes the current state of the keyboard, and updates state as
	// necessary.
	void processKeyboard(std::string &line);

	// Starts processing the keyboard every frame until the Enter key is
	// pressed, then writes user input into `line`.
	void getLine(std::string &line);

	inline constexpr bool newlineEntered() const { return _newlineEntered; }
	inline constexpr bool foldPressed() const { return _foldPressed; }

	inline constexpr const std::vector<std::string> &getLineHistory()
	{
		return lineHistory;
	}

	void setLineHistory(const std::string &filename);
	inline constexpr void clearLineHistory() { lineHistory.clear(); }
};
