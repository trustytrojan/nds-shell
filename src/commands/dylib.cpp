// this file is only compiled with BlocksDS

#include "Commands.hpp"
#include "DylibManager.hpp"
#include <dlfcn.h>
#include <expected>

void Commands::dylib(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "usage: dylib <dsl-path>\n";
		return;
	}

	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mdylib: fs not initialized\e[m\n";
		return;
	}

	{ // scope so that the `Handle` object unrefs the library before `gc()`
		const auto handle_ex{DylibManager::open_lib(ctx.args[1])};
		if (!handle_ex)
		{
			ctx.err << "\e[91mdylib: dlopen: " << handle_ex.error() << "\e[m\n";
			return;
		}

		const auto fn = reinterpret_cast<VoidFn>(dlsym(handle_ex->get(), "ndsh_dylib_demo"));
		if (!fn)
		{
			ctx.err << "\e[91mdylib: dlsym: " << dlerror() << "\e[m\n";
			return;
		}

		fn();
	}

	DylibManager::gc();
}
