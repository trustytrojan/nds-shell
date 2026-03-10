// this file is only compiled with BlocksDS

#include "Commands.hpp"
#include <dlfcn.h>
#include <expected>
#include <span>
#include <utility>

class DynamicLibrary
{
	void *handle;

public:
	constexpr DynamicLibrary(void *handle)
		: handle{handle}
	{
	}

	DynamicLibrary(const DynamicLibrary &) = delete;
	DynamicLibrary &operator=(const DynamicLibrary &) = delete;

	constexpr DynamicLibrary(DynamicLibrary &&other)
	{
		close();
		handle = std::exchange(other.handle, {});
	}

	constexpr DynamicLibrary &operator=(DynamicLibrary &&other)
	{
		close();
		handle = std::exchange(other.handle, {});
		return *this;
	}

	constexpr ~DynamicLibrary() { close(); }

	constexpr void close()
	{
		if (!handle)
			return;
		dlclose(handle);
		handle = {};
	}

	constexpr auto get_symbol(const char *name) const { return dlsym(handle, name); }
	constexpr operator void *() const { return handle; }
};

auto read_dep_names(FILE *const f) -> std::expected<std::vector<std::string>, std::string>
{
	int dep_count;

	if (fread(&dep_count, sizeof(int), 1, f) != 1)
		return std::unexpected{"failed to read dep count"};

	if (dep_count == 0)
		return {};

	std::vector<std::string> dep_names;

	for (int i{}; i < dep_count; ++i)
	{
		std::string name;

		// The strings are null-terminated, so this is fine.
		while (const auto c{fgetc(f)})
		{
			if (c == EOF)
				return std::unexpected{"end of file reached"};
			name += c;
		}

		dep_names.emplace_back(std::move(name));
	}

	return dep_names;
}

auto dlopen_FILE_with_deps(FILE *f, int mode, std::span<void *const> deps)
{
	return dlopen_FILE_with_deps(f, mode, deps.data(), deps.size());
}

static std::vector<std::string_view> loading_libs(0);

auto is_loading(std::string_view lib)
{
	return std::ranges::find(loading_libs, lib) != loading_libs.end();
}

auto open_dylib_autodep(std::string_view path) -> std::expected<void *, std::string>
{
	if (!path.ends_with(".dsl"))
		return std::unexpected{"not a .dsl file"};

	const auto file{fopen(path.data(), "r")};
	if (!file)
		return std::unexpected{"failed to open file"};

	const auto dep_names{read_dep_names(file)};
	if (!dep_names)
		return std::unexpected{"read_dep_names: " + dep_names.error()};

	// Emplace name without ".dsl"
	const auto name = path.substr(0, path.size() - 4);
	loading_libs.emplace_back(name);

	auto dep_libs = std::vector<void *>{};
	for (const auto dep_name : *dep_names)
	{
		if (is_loading(dep_name))
		{
			loading_libs.pop_back();
			return std::unexpected{"circular dependency with '" + dep_name + "'"};
		}

		const auto dep_lib{open_dylib_autodep(dep_name + ".dsl")};
		if (!dep_lib)
		{
			loading_libs.pop_back();
			return std::unexpected{"dep '" + dep_name + "': " + dep_lib.error()};
		}

		dep_libs.emplace_back(*dep_lib);
	}

	loading_libs.pop_back();

	const auto handle{dlopen_FILE_with_deps(file, RTLD_NOW | RTLD_LOCAL, dep_libs)};
	fclose(file);

	if (!handle)
		return std::unexpected{dlerror()};

	return handle;
}

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

	const auto handle{open_dylib_autodep(ctx.args[1])};
	if (!handle)
	{
		ctx.err << "\e[91mdylib: dlopen: " << handle.error() << "\e[m\n";
		return;
	}

	const DynamicLibrary lib{*handle};

	const auto fn = reinterpret_cast<VoidFn>(lib.get_symbol("ndsh_dylib_demo"));
	if (!fn)
	{
		ctx.err << "\e[91mdylib: dlsym: " << dlerror() << "\e[m\n";
		return;
	}

	fn();
}
