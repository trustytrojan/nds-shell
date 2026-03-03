#include <array>

namespace
{
std::array<int, 4> offsets = {7, 11, 13, 17};
}

extern "C" __attribute__((visibility("default"))) int ndsh_dylib_demo(int value)
{
	int index = value & 3;
	return value * 2 + offsets[index];
}
