#include <cstdio>
#include <cstdlib>

// this is the workaround for a [[noreturn]] function. do NOT mark it `const`!
auto ndsh_exit = exit;

extern "C" void ndsh_dylib_demo()
{
	ndsh_exit(67);
	puts("demo: after exit");
}
