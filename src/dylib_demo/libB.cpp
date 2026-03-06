#include <cstdio>

extern "C" __attribute__((visibility("default"))) void libB_print()
{
	puts("libB called");
}

extern "C" __attribute__((visibility("default"))) void ndsh_dylib_demo()
{
	libB_print();
}
