#pragma once

#include <calico/types.h>
#include <iostream>
#include <string>

// https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797
namespace EscapeSequences
{

const auto BASE = "\e[";

namespace Cursor
{

enum class MoveDirection
{
	UP = 'A',
	DOWN,
	RIGHT,
	LEFT
};

inline auto move(const MoveDirection direction, const int amount)
{
	return BASE + std::to_string(amount) + (char)direction;
}

const auto moveLeftOne = "\e[1D", moveRightOne = "\e[1C";

} // namespace Cursor

} // namespace EscapeSequences

struct ColorString
{
	int color;
	const char *const str;
};

#define ColorString_op(color, number)                                   \
	inline ColorString operator"" _##color(const char *const s, size_t) \
	{                                                                   \
		return {number, s};                                             \
	}

ColorString_op(green, 32);
ColorString_op(brightred, 41);
ColorString_op(brightcyan, 46);
ColorString_op(blue, 44)

inline std::ostream &operator<<(std::ostream &ostr, const ColorString &c)
{
	return ostr << "\e[" << c.color << 'm' << c.str << "\e[39m";
}
