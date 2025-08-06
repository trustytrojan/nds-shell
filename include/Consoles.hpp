#pragma once

#include <nds.h>

#include <fstream>

namespace Consoles
{

enum class Display
{
	TOP,
	BOTTOM
};

const int NUM_CONSOLES = 7;

inline PrintConsole consoles[NUM_CONSOLES];
inline u8 focused_console{}, top_shown_console{}, bottom_shown_console{4};
inline Display focused_display{};
inline std::ofstream streams[NUM_CONSOLES];

void Init();
void InitMulti();

void toggleFocusedDisplay();
void handleKeyL();
void handleKeyR();

} // namespace Consoles
