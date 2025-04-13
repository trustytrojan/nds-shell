#pragma once

#include <nds.h>

#include <string>
#include <vector>

// Commandline prompt with a visible, movable cursor that can edit the line at
// any position.
struct CliPrompt
{
	CliPrompt(
		const std::string &basePrompt, const char cursor, std::ostream &ostr);

	// Starts processing the keyboard every frame until the Enter key is
	// pressed, then writes user input into `line`.
	void GetLine(std::string &line);

	// The text to print before the cursor.
	std::string basePrompt;

	// The character to print as the cursor.
	char cursor;

	// The stream to print to.
	std::ostream &ostr;

	/* Future features!
		// Autocomplete function
		std::function<std::string(const std::string &)> autoComplete;
	*/
	// Line history
	std::vector<std::string> lineHistory;
	std::vector<std::string>::const_iterator lineHistoryItr;

private:
	// Processes the current state of the keyboard, and updates state as
	// necessary.
	void ProcessKeyboard(
		std::string &line,
		u32 &cursorPos,
		bool &flashState,
		u8 &flashTimer,
		bool &newlineEntered);
};
