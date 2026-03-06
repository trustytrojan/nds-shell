#include <cstdio>

extern "C" __attribute__((visibility("default"))) int ndsh_libB_value(int value)
{
	std::printf("libB: ndsh_libB_value(%d)\n", value);
	return value + 10;
}

extern "C" __attribute__((visibility("default"))) int ndsh_dylib_demo(int value)
{
	std::puts("libB ndsh_dylib_demo called!");
	return 67;
}

