#include <cstdio>

extern "C" void libB_print();
extern "C" void libC_print();

extern "C" void ndsh_dylib_demo()
{
	puts("libA calling libB...");
	libB_print();
	puts("libA calling libC...");
	libC_print();
}
