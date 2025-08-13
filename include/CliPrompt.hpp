#pragma once

#include <nds.h>

#include <iostream>
#include <string>
#include <vector>

struct MyTickTask : TickTask
{
	bool &flashState;
};

// Asynchronous command-line interface prompt, taking input from the
// keypad (physical buttons) and libnds virtual keyboard. Provides:
// - a visible, movable cursor that can edit the line at any position
// - input line history with arrow-key & d-pad button navigation
class CliPrompt
{
	std::ostream *ostr = &std::cout;
	std::string prompt = "> ";

	// The input buffer, also referred to as the line buffer.
	std::string input;

	// State necessary for rendering a visible cursor
	u32 cursorPos{}; // relative to `input`
	bool flashState{};

	// Keypress state. Reset on every call to `processKeyboard()`.
	bool _enterPressed{}, _foldPressed{};

	std::vector<std::string> lineHistory;
	// an integer could be easier... but it's already done so whatever.
	std::vector<std::string>::const_iterator lineHistoryItr;

	// Saves `line` when navigating up the line history to save what
	// the user currently entered as the "new" line to be saved.
	// Restores `line` when the user navigates all the way to one
	// past the end of the line history.
	std::string savedInput;

	void resetKeypressState() { _enterPressed = _foldPressed = {}; }
	void flashCursor();
	void handleBackspace();
	void handleEnter();
	void handleLeft();
	void handleRight();
	void handleUp();
	void handleDown();

	// Returns whether an event was run via a keypad press.
	bool processKeypad();

	// Processes the current state of the keyboard/keypad, updating state as
	// necessary.
	void processKeyboard();

public:
	CliPrompt() { prepareForNextLine(); }

	// The output stream to print to.
	void setOutputStream(std::ostream &o) { ostr = &o; }

	// Set the text to print before the cursor.
	void setPrompt(const std::string &s) { prompt = s; }

	// Set the character to print as the cursor.
	// void setCursor(char c) { cursor = c; }

	// Read the input buffer.
	const std::string &getInput() const { return input; }

	// Print the prompt and cursor, optionally with the input buffer in between.
	void printFullPrompt(bool withInput);

	// Clears state associated with the last input captured, including the input
	// itself.
	void prepareForNextLine();

	// Processes the current state of the keypad, then the keyboard, and updates
	// state as necessary.
	void update();

	// Returns whether the `Enter` key was pressed during the last call to
	// `processKeyboard()`. This also indicates that a newline character (`\n`)
	// was written to the output stream.
	bool enterPressed() const { return _enterPressed; }

	// Returns whether the "fold" key (in the same place as the `Esc` key) was
	// pressed during the last call to `processKeyboard()`.
	bool foldPressed() const { return _foldPressed; }

	// View the current line history.
	const std::vector<std::string> &getLineHistory() const { return lineHistory; }

	// Set the current line history using the contents of the file located at
	// `filename`.
	void setLineHistoryFromFile(const std::string &filename);

	// Clear the line history.
	void clearLineHistory() { lineHistory.clear(); }
};
