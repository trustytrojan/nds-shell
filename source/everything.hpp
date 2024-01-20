#ifndef __EVERYTHING__
#define __EVERYTHING__

#include <nds.h>
#include <fat.h>
#include <dswifi9.h>

#include <filesystem>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>

typedef std::vector<std::string> Args;
typedef void (*CommandFunction)(const Args &);
typedef std::function<void(const std::string &)> ShellLineProcessor;

extern Keyboard *keyboard;

/**
 * Start a shell prompt with a visible cursor and cursor controls.
 * \param prompt The text to print before the cursor. For example `prompt = "> "` and 
 * 				 `cursorCharacter = '_'` would result in the initial shell prompt looking like `> _`.
 * \param cursorCharacter The character to use as the terminal cursor.
 * \param lineProcessor A function to accept lines entered by the user. Cannot be null.
 * \param env Environment variables. Defaults to empty dictionary.
 */
void StartShell(std::string prompt, const char cursorCharacter, const ShellLineProcessor lineProcessor);

/**
 * Compatible with `StartShell`. Parses whitespace-separated tokens into a `std::vector<std::string>`, then passes it to `RunCommand`.
 */
void ProcessLine(const std::string &line);

/**
 * With `args` from `ProcessLine`, runs a defined command.
 */
void RunCommand(const Args &args);

#endif