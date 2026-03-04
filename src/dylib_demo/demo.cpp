#include <iostream>

extern "C" __attribute__((visibility("default"))) int ndsh_dylib_demo(int value)
{
	puts("puts"); // puts
	printf("printf\n"); // printf
	printf("stdout: %p\n", stdout); // 0
	fprintf(stdout, "fprintf(stdout)\n");
	printf("stderr: %p\n", stderr); // 0
	fprintf(stderr, "fprintf(stderr)\n");
	printf("cout: %p\n", &std::cout); // same as dlmembase()
	std::cout << "cout print\n";
	printf("cerr: %p\n", &std::cerr); // same as dlmembase()
	std::cerr << "cerr print\n";
	// fprintf(stdout, "fprintf(stdout)\n"); // causes ARM9 data abort
	// fprintf(stderr, "fprintf(stderr)\n"); // causes ARM9 data abort
	// std::cout << "cout" << std::endl; // causes ARM9 data abort
	// std::cerr << "cerr" << std::endl; // causes ARM9 data abort
	return 65; // with the above 4 lines commented out, this is safely returned
}
