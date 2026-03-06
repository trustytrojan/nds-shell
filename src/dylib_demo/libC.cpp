#include <cstdio>

extern "C" __attribute__((visibility("default"))) int ndsh_libC_value(int value)
{
	std::printf("libC: ndsh_libC_value(%d)\n", value);
	return value + 20;
}

extern "C" __attribute__((visibility("default"))) int ndsh_dylib_demo(int value)
{
	std::puts("libC ndsh_dylib_demo called!");
	return 69;
}
