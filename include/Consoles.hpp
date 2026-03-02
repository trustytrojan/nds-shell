#pragma once

#include <nds.h>

#include <iostream>

namespace Consoles
{

void Init();

#ifdef NDSH_MULTICONSOLE
enum class Display
{
	TOP,
	BOTTOM
};

const int NUM_CONSOLES = 3;

void toggleFocusedDisplay();
void switchConsoleLeft();
void switchConsoleRight();

u8 GetFocusedConsole();
bool IsFocused(u8 console);
std::ostream &GetStream(u8 console);
#else
constexpr u8 GetFocusedConsole()
{
	return 0;
}
#endif

} // namespace Consoles
