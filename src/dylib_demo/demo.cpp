// #include <cstdlib>
extern "C" void ndsh_dylib_demo()
{
	[[noreturn]] void exit(int);
	exit(67);
}
