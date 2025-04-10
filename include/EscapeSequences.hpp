#pragma once

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

auto move(const MoveDirection direction, const int n)
{
	return BASE + std::to_string(n) + (char)direction;
}

const auto moveLeftOne = move(MoveDirection::LEFT, 1);
const auto moveRightOne = move(MoveDirection::RIGHT, 1);
} // namespace Cursor
} // namespace EscapeSequences
