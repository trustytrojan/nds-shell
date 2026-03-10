// this file is only compiled with BlocksDS

#include "Commands.hpp"
#include <array>
#include <dlfcn.h>
#include <expected>
#include <span>
#include <string>
#include <utility>

class DynamicLibrary
{
public:
	DynamicLibrary(const DynamicLibrary &) = delete;
	DynamicLibrary &operator=(const DynamicLibrary &) = delete;

	DynamicLibrary(DynamicLibrary &&other) noexcept
		: handle{std::exchange(other.handle, nullptr)}
	{
	}

	DynamicLibrary &operator=(DynamicLibrary &&other) noexcept
	{
		if (this == &other)
			return *this;

		close();
		handle = std::exchange(other.handle, nullptr);
		return *this;
	}

	static std::expected<DynamicLibrary, std::string> open(const char *path)
	{
		dlerror();
		void *const handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
		if (!handle)
		{
			if (const char *const error = dlerror())
				return std::unexpected{std::string{error}};
			return std::unexpected{std::string{"unknown error"}};
		}

		return DynamicLibrary{handle};
	}

	static std::expected<DynamicLibrary, std::string>
	open_with_deps(const char *path, std::span<void *const> depHandles)
	{
		dlerror();
		void *const handle =
			dlopen_with_deps(path, RTLD_NOW | RTLD_LOCAL, depHandles.data(), static_cast<int>(depHandles.size()));
		if (!handle)
		{
			if (const char *const error = dlerror())
				return std::unexpected{std::string{error}};
			return std::unexpected{std::string{"unknown error"}};
		}

		return DynamicLibrary{handle};
	}

private:
	void *handle{};

	DynamicLibrary(void *const handle)
		: handle{handle}
	{
	}

	void close() noexcept
	{
		if (!handle)
			return;

		dlclose(handle);
		handle = nullptr;
	}

public:
	~DynamicLibrary() { close(); }

	[[nodiscard]] void *native_handle() const { return handle; }

	std::expected<void *, std::string> get_symbol(const char *name)
	{
		dlerror();
		void *const sym = dlsym(handle, name);
		if (const char *const error = dlerror())
			return std::unexpected{std::string{error}};
		return sym;
	}
};

void Commands::dylib(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mdylib: fs not initialized\e[39m\n";
		return;
	}

	auto libBEx = DynamicLibrary::open("B.dsl");
	if (!libBEx)
	{
		ctx.err << "\e[91mdylib: dlopen(B): " << libBEx.error() << "\e[39m\n";
		return;
	}
	DynamicLibrary libB = std::move(*libBEx);

	auto libCEx = DynamicLibrary::open("C.dsl");
	if (!libCEx)
	{
		ctx.err << "\e[91mdylib: dlopen(C): " << libCEx.error() << "\e[39m\n";
		return;
	}
	DynamicLibrary libC = std::move(*libCEx);

	std::array<void *, 2> depHandles{libB.native_handle(), libC.native_handle()};
	auto libAEx = DynamicLibrary::open_with_deps("A.dsl", depHandles);
	if (!libAEx)
	{
		ctx.err << "\e[91mdylib: dlopen_with_deps(A): " << libAEx.error() << "\e[39m\n";
		return;
	}
	DynamicLibrary libA = std::move(*libAEx);

	auto fnEx = libA.get_symbol("ndsh_dylib_demo");
	if (!fnEx)
	{
		ctx.err << "\e[91mdylib: dlsym(ndsh_dylib_demo): " << fnEx.error() << "\e[39m\n";
		return;
	}
	auto fn = reinterpret_cast<VoidFn>(*fnEx);

	ctx.out << "dylib: calling into libA...\n";
	fn();
}
