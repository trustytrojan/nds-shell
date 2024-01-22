#ifndef __PROMPT__
#define __PROMPT__

#include "libdeps.hpp"

// Command-line prompt.
struct CliPrompt
{
	CliPrompt(const std::string &basePrompt, const char cursor, std::ostream &ostr);

	// Starts processing the keyboard every frame until the Enter key is pressed, then writes the user input into `line`.
	// This method does not clear `line` before adding user input.
	void GetLine(std::string &line) const;

	// The text to print before the cursor.
	std::string basePrompt;

	// The character to print as the cursor.
	char cursor;

	// The stream to print to.
	std::ostream &ostr;

private:
	// Processes the current state of the keyboard, and updates state as necessary.
	void ProcessKeyboard(std::string &line, u32 &cursorPos, bool &flashState, u8 &flashTimer, bool &newlineEntered) const;
};

#endif