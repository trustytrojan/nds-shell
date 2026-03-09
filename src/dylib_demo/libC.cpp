#include <cstdio>

extern "C" __attribute__((visibility("default"))) void libC_print()
{
	puts("libC called");
}

extern "C" __attribute__((visibility("default"))) void ndsh_dylib_demo()
{
	libC_print();
	char *curl_version(void);
	// puts(curl_version());
}
