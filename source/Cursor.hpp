#ifndef __CURSOR__
#define __CURSOR__

#include <iostream>

// Base string for an ANSI escape sequence
const auto ESC_SEQ_BASE = "\e[";

// Love this guy https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
namespace Cursor
{
	enum MoveDirection
	{
		UP = 'A',
		DOWN,
		RIGHT,
		LEFT
	};

	auto move(const MoveDirection direction, const int n)
	{
		return ESC_SEQ_BASE + std::to_string(n) + (char)direction;
	}

	// Optimizations
	const auto moveLeftOne = move(LEFT, 1);
	const auto moveRightOne = move(RIGHT, 1);
}

#endif