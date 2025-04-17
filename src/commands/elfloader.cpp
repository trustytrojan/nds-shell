#include "elfload.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
// #include <sys/mman.h>
#include <iostream>

#include "Commands.hpp"
#include "Shell.hpp"

FILE *f;
void *buf;

typedef void (*entrypoint_t)(int (*putsp)(const char *));

static bool fpread(el_ctx *ctx, void *dest, size_t nb, size_t offset)
{
	(void)ctx;

	if (fseek(f, offset, SEEK_SET))
		return false;

	if (fread(dest, nb, 1, f) != 1)
		return false;

	return true;
}

static void *alloccb(el_ctx *ctx, Elf_Addr phys, Elf_Addr virt, Elf_Addr size)
{
	(void)ctx;
	(void)phys;
	(void)size;
	return (void *)virt;
}

static void check(el_status stat, const char *expln)
{
	if (stat)
	{
		fprintf(stderr, "%s: error %d\n", expln, stat);
		exit(1);
	}
}

static void go(entrypoint_t ep)
{
	ep(puts);
}

void Commands::elfloader()
{
	if (Shell::args.size() < 2)
	{
		fprintf(stderr, "elf executable required\n");
		return;
	}

	f = fopen(Shell::args[1].c_str(), "rb");
	if (!f)
	{
		perror("fopen");
		return;
	}

	el_ctx ctx;
	ctx.pread = fpread;

	std::cout << "initializing\n";
	check(el_init(&ctx), "initialising");

	// if (posix_memalign(&buf, ctx.align, ctx.memsz))
	// {
	// 	perror("memalign");
	// 	return;
	// }

	// if (mprotect(buf, ctx.memsz, PROT_READ | PROT_WRITE | PROT_EXEC))
	// {
	// 	perror("mprotect");
	// 	return 1;
	// }

	ctx.base_load_vaddr = ctx.base_load_paddr = (uintptr_t)buf;

	std::cout << "loading\n";
	check(el_load(&ctx, alloccb), "loading");
	// check(el_relocate(&ctx), "relocating");

	uintptr_t epaddr = ctx.ehdr.e_entry + (uintptr_t)buf;

	entrypoint_t ep = (entrypoint_t)epaddr;

	printf(
		"Binary entrypoint is %" PRIxPTR "; invoking %p\n",
		(uintptr_t)ctx.ehdr.e_entry,
		ep);

	std::cout << "going\n";
	go(ep);

	fclose(f);

	free(buf);
}
