#pragma once

#include <nds.h>

#include <iostream>

namespace Consoles
{

enum class Display
{
	TOP,
	BOTTOM
};

const int NUM_CONSOLES = 3;

void Init();
void InitMulti();

void toggleFocusedDisplay();
void switchConsoleLeft();
void switchConsoleRight();

u8 GetFocusedConsole();
bool IsFocused(u8 console);
std::ostream &GetStream(u8 console);

} // namespace Consoles
