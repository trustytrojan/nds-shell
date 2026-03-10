#include <cstdio>

extern "C" void libB_print()
{
	puts("libB called");
}

extern "C" void ndsh_dylib_demo()
{
	libB_print();
}
