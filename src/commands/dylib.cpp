// this file is only compiled with BlocksDS

#include "Commands.hpp"
#include <dlfcn.h>

void Commands::dylib(const Context &ctx)
{
	if (ctx.args.size() < 2 || ctx.args.size() > 4)
	{
		ctx.err << "usage: dylib <dsl-path>\n";
		return;
	}

	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mdylib: fs not initialized\e[39m\n";
		return;
	}

	const auto &dslPath = ctx.args[1];
	const char *symbolName = "ndsh_dylib_demo";
	if (ctx.args.size() >= 3)
		symbolName = ctx.args[2].c_str();

	void *handle = dlopen(dslPath.c_str(), RTLD_NOW | RTLD_LOCAL);
	if (const auto error{dlerror()})
	{
		ctx.err << "\e[91mdylib: dlopen: " << error << "\e[39m\n";
		return;
	}

	auto fn = reinterpret_cast<VoidFn>(dlsym(handle, symbolName));
	if (const auto error{dlerror()})
	{
		ctx.err << "\e[91mdylib: dlsym: " << error << "\e[39m\n";
		dlclose(handle);
		dlerror();
		return;
	}

	ctx.out << "dylib: calling into library...\n";
	fn();

	if (dlclose(handle) != 0)
	{
		if (const auto error{dlerror()})
			ctx.err << "\e[91mdylib: dlclose: " << error << "\e[39m\n";
	}
}
