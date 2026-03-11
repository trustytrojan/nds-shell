#pragma once

#include <expected>
#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

// Reference-count-based dynamic library manager with manual garbage collection.
namespace DylibManager
{

// Unref a library by its `void *` handle.
auto unref_lib(void *handle) -> void;

// `unique_ptr` wrapper over the `void *` handle from `dlopen()`.
// Unrefs itself on destruction.
struct Handle : std::unique_ptr<void, decltype(&unref_lib)>
{
	constexpr Handle(void *ptr)
		: std::unique_ptr<void, decltype(&unref_lib)>{ptr, unref_lib}
	{
	}
};

// Open a library by filepath.
// Once library search paths are added, you can simply pass a library name.
// If a library is still loaded in memory, it will be ref'd and returned.
auto open_lib(const fs::path &path) -> std::expected<Handle, std::string>;

// Invoke the garbage collector. Collects libraries with zero references.
// Libraries *can* be referenced by dependent libraries.
auto gc() -> void;

} // namespace DylibManager
