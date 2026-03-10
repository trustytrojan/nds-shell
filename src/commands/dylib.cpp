// this file is only compiled with BlocksDS

#include "Commands.hpp"
#include <dlfcn.h>
#include <span>

struct DynamicLibrary
{
	void *const handle;

	constexpr DynamicLibrary(std::string_view path)
		: handle{dlopen(path.data(), RTLD_NOW | RTLD_LOCAL)}
	{
	}

	constexpr DynamicLibrary(std::string_view path, std::span<void *const> dep_handles)
		: handle{dlopen_with_deps(path.data(), RTLD_NOW | RTLD_LOCAL, dep_handles.data(), dep_handles.size())}
	{
	}

	constexpr ~DynamicLibrary() { dlclose(handle); }
	constexpr void *get_symbol(const char *name) const { return dlsym(handle, name); }
	constexpr operator void *() const { return handle; }
};

void Commands::dylib(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mdylib: fs not initialized\e[39m\n";
		return;
	}

	const DynamicLibrary libB{"B.dsl"};
	if (!libB)
	{
		ctx.err << "\e[91mdylib: dlopen(B): " << dlerror() << "\e[39m\n";
		return;
	}

	const DynamicLibrary libC{"C.dsl"};
	if (!libC)
	{
		ctx.err << "\e[91mdylib: dlopen(C): " << dlerror() << "\e[39m\n";
		return;
	}

	const DynamicLibrary libA{"A.dsl", {libB, libC}};
	if (!libA)
	{
		ctx.err << "\e[91mdylib: dlopen(A): " << dlerror() << "\e[39m\n";
		return;
	}

	const auto fn = (VoidFn)libA.get_symbol("ndsh_dylib_demo");
	if (!fn)
	{
		ctx.err << "\e[91mdylib: dlsym(ndsh_dylib_demo): " << dlerror() << "\e[39m\n";
		return;
	}

	ctx.out << "dylib: calling into libA...\n";
	fn();
}
