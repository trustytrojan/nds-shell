#include "Commands.hpp"

#ifdef __BLOCKSDS__
	#include <dlfcn.h>
#endif

#include <cstdlib>

void Commands::dylib(const Context &ctx)
{
	if (ctx.args.size() < 2 || ctx.args.size() > 4)
	{
		ctx.err << "usage: dylib <dsl-path> [symbol] [int-arg]\n";
		return;
	}

#ifndef __BLOCKSDS__
	ctx.err << "\e[91mdylib: only available with BlocksDS builds\e[39m\n";
	return;
#else
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mdylib: fs not initialized\e[39m\n";
		return;
	}

	const auto &dslPath = ctx.args[1];
	const char *symbolName = "ndsh_dylib_demo";
	if (ctx.args.size() >= 3)
		symbolName = ctx.args[2].c_str();

	char *end = nullptr;
	long inputValue = 123;
	if (ctx.args.size() == 4)
	{
		inputValue = std::strtol(ctx.args[3].c_str(), &end, 10);
		if (end == ctx.args[3].c_str() || *end != '\0')
		{
			ctx.err << "\e[91mdylib: invalid integer argument: " << ctx.args[3] << "\e[39m\n";
			return;
		}
	}

	void *handle = dlopen(dslPath.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (const char *error = dlerror(); error)
	{
		ctx.err << "\e[91mdylib: dlopen: " << error << "\e[39m\n";
		return;
	}

	ctx.out << "loaded: " << dslPath << "\n"
			<< "membase: " << dlmembase(handle) << '\n';

	dlerror();
	using DemoFn = int (*)(int);
	auto fn = reinterpret_cast<DemoFn>(dlsym(handle, symbolName));
	if (const char *error = dlerror(); error)
	{
		ctx.err << "\e[91mdylib: dlsym(" << symbolName << "): " << error << "\e[39m\n";
		dlclose(handle);
		dlerror();
		return;
	}

	const int result = fn(static_cast<int>(inputValue));
	ctx.out << "call: " << symbolName << '(' << inputValue << ") = " << result << '\n';

	if (dlclose(handle) != 0)
	{
		if (const char *error = dlerror(); error)
			ctx.err << "\e[91mdylib: dlclose: " << error << "\e[39m\n";
	}
	else
	{
		ctx.out << "unloaded library\n";
	}
#endif
}