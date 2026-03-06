#include <cstdio>

extern "C" int ndsh_libB_value(int value);
extern "C" int ndsh_libC_value(int value);

extern "C" __attribute__((visibility("default"))) int ndsh_dylib_demo(int value)
{
	std::printf("libA: ndsh_dylib_demo(%d)\n", value);
	const int fromB = ndsh_libB_value(value);
	const int fromC = ndsh_libC_value(value);
	std::printf("libA: from libB=%d, from libC=%d\n", fromB, fromC);
	return fromB + fromC;
}
