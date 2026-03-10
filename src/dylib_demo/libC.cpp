#include <cstdio>

extern "C" void libC_print()
{
	puts("libC called");
}

extern "C" void ndsh_dylib_demo()
{
	libC_print();
	// char *curl_version(void);
	// puts(curl_version());
}
