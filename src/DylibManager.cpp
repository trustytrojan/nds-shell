#include "DylibManager.hpp"
#include <algorithm>
#include <cstring>
#include <dlfcn.h>
#include <iostream>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

using namespace std::literals;

static auto read_dep_names(FILE *const f) -> std::expected<std::vector<std::string>, std::string>
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

std::span<void *const> deps;

// THIS is overridable!
extern "C" void my_func(int x)
{
	std::cerr << "my_func called with x=" << x << '\n';
}

static void my_other_func(int x)
{
	printf("my_other_func: x=%d\n", x);
}

constexpr auto lib_func = "exit"sv;

static void dep_symbol_resolver(const char *const name, uint32_t *const value, const uint32_t attributes)
{
	// Override a symbol's value with our own function!
	if (name == lib_func)
	{
		std::cerr << "current " << lib_func << " value: " << (void *)*value << '\n';
		*value = (uint32_t)my_other_func;
		std::cerr << "set " << lib_func << " value to " << (void *)my_other_func << '\n';
		return;
	}

	// Only resolve symbols marked unresolved by dsltool.
	// This means already-resolved main binary symbols won't pass this.
	if (!(attributes & DSL_SYMBOL_UNRESOLVED))
		return;

	// Search all dependencies for the symbol's name.
	for (const auto dep : deps)
		if ((*value = (uint32_t)dlsym(dep, name)))
			// dlsym() did not return NULL, so this dependency had the symbol!
			break;
}

// this mitigates the -Wignored-attributes caused by decltype(&fclose)
struct FileCloser
{
	auto operator()(FILE *f) const noexcept -> void
	{
		if (f)
			(void)fclose(f);
	}
};

static auto normalize_lib_path(fs::path path) -> std::expected<fs::path, std::string>
{
	if (path.extension().empty())
		path += ".dsl";
	else if (path.extension() != ".dsl")
		return std::unexpected{"must be a .dsl file"};

	return path;
}

namespace DylibManager
{

static std::vector<std::string> loading_libs;
static constexpr auto lib_is_loading(std::string_view lib)
{
	return std::ranges::find(loading_libs, lib) != loading_libs.end();
}

struct LoadedLib
{
	std::string name;
	void *handle;
	int refs;
	std::vector<Handle> deps;
};
static std::vector<LoadedLib> loaded_libs;
static constexpr auto find_loaded_lib(std::string_view name)
{
	return std::ranges::find_if(loaded_libs, [&](auto &lib) { return lib.name == name; });
}
static constexpr auto find_loaded_lib(void *handle)
{
	return std::ranges::find_if(loaded_libs, [=](auto &lib) { return lib.handle == handle; });
}

auto unref_lib(void *handle) -> void
{
	if (!handle)
		return;
	const auto lib{find_loaded_lib(handle)};
	if (lib == loaded_libs.end() || lib->refs <= 0)
		return;
	--lib->refs;
	std::cerr << "unref'd lib '" << lib->name << "': refs=" << lib->refs << '\n';
}

auto open_lib(const fs::path &path) -> std::expected<Handle, std::string>
{
	const auto normalized_path{normalize_lib_path(path)};
	if (!normalized_path)
		return std::unexpected{normalized_path.error()};

	const auto lib_name{normalized_path->string()};

	if (const auto it{find_loaded_lib(lib_name)}; it != loaded_libs.end())
	{
		++it->refs;
		std::cerr << "lib '" << lib_name << "' already loaded: refs=" << it->refs << '\n';
		return it->handle;
	}

	std::unique_ptr<FILE, FileCloser> file{fopen(lib_name.c_str(), "r")};
	if (!file)
		return std::unexpected{"failed to open file"};

	const auto dep_names{read_dep_names(file.get())};
	if (!dep_names)
		return std::unexpected{"read_dep_names: " + dep_names.error()};

	loading_libs.emplace_back(lib_name);

	// this is the killer. since Handle objects are `unique_ptr`s,
	// if we need to error-return for any reason, all deps in here
	// will properly unref themselves!
	std::vector<Handle> dep_handles;

	for (const auto &dep_name : *dep_names)
	{
		const auto dep_path{normalize_lib_path(normalized_path->parent_path() / dep_name)};
		if (!dep_path)
		{
			loading_libs.pop_back();
			return std::unexpected{"dep '" + dep_name + "': " + dep_path.error()};
		}

		const auto dep_name_normalized{dep_path->string()};
		if (lib_is_loading(dep_name_normalized))
		{
			loading_libs.pop_back();
			return std::unexpected{"circular dependency with '" + dep_name_normalized + "'"};
		}

		auto dep_lib{open_lib(*dep_path)};
		if (!dep_lib)
		{
			loading_libs.pop_back();
			return std::unexpected{"dep '" + dep_name + "': " + dep_lib.error()};
		}

		dep_handles.emplace_back(std::move(*dep_lib));
	}

	loading_libs.pop_back();

	// map handles to their pointers
	const auto dep_ptrs{dep_handles | std::views::transform(&Handle::get) | std::ranges::to<std::vector>()};

	// TODO: move this to an "init" function
	dsl_set_symbol_resolver(dep_symbol_resolver);

	// give the deps to our symbol resolver
	deps = dep_ptrs;

	// actual library opening
	const auto handle{dlopen_FILE(file.get(), RTLD_NOW | RTLD_LOCAL)};
	if (!handle)
		return std::unexpected{dlerror()};

	// store a new LoadedLib, moving all our dependency `Handle`s into it!
	const auto &lib = loaded_libs.emplace_back(lib_name, handle, 1, std::move(dep_handles));

	std::cerr << "opened lib '" << lib_name << "': refs=" << lib.refs << '\n';

	return handle;
}

// only meant to be used by gc()
static auto erase_lib(const std::vector<LoadedLib>::iterator lib)
{
	std::cerr << "collecting lib '" << lib->name << "'\n";
	dlclose(lib->handle);
	for (const auto &dep : lib->deps)
		unref_lib(dep.get());
	// continue iteration of gc() loop
	return loaded_libs.erase(lib);
}

auto gc() -> void
{
	bool erased_any;
	do
	{
		erased_any = false;
		for (auto it{loaded_libs.begin()}; it != loaded_libs.end();)
		{
			if (it->refs > 0)
			{
				++it;
				continue;
			}

			it = erase_lib(it);
			erased_any = true;
		}
	} while (erased_any);
}

} // namespace DylibManager
