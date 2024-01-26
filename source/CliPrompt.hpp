#pragma once

#include "libdeps.hpp"

// Command-line prompt with a visible, movable cursor that can edit the line at any position.
struct CliPrompt
{
	CliPrompt(const std::string &basePrompt, const char cursor, std::ostream &ostr);

	// Starts processing the keyboard every frame until the Enter key is pressed, then writes user input into `line`.
	void GetLine(std::string &line) const;

	// The text to print before the cursor.
	std::string basePrompt;

	// The character to print as the cursor.
	char cursor;

	// The stream to print to.
	std::ostream &ostr;

/* Future feature!
	// Autocomplete function
	std::function<std::string(const std::string &)> autoComplete;
*/

private:
	// Processes the current state of the keyboard, and updates state as necessary.
	void ProcessKeyboard(std::string &line, u32 &cursorPos, bool &flashState, u8 &flashTimer, bool &newlineEntered) const;
};