#include "Shell.hpp"

int main()
{
	// Never buffer streams! Save memory.
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);

	// Guru Meditation exception handler
	defaultExceptionHandler();

	Shell::Start();
}
