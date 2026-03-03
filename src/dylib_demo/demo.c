#define SYM_PUBLIC __attribute__((visibility("default")))

static int offsets[4] = {7, 11, 13, 17};

SYM_PUBLIC int ndsh_dylib_demo(int value)
{
	int index = value & 3;
	return value * 2 + offsets[index];
}