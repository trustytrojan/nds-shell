# BlocksDS Dynamic Libraries (DSL) in nds-shell

This project provides a reusable CMake helper in:

- `cmake/BlocksDSDynamicLibraries.cmake`

Use it to build one or more BlocksDS dynamic libraries (`.dsl`) from C++ sources.

## Why a helper function exists

CMake `add_library()` does not allow arbitrary custom library types for project-built artifacts.
For DSLs, we need a custom pipeline:

1. compile sources to objects
2. link objects into an ELF with DSL-compatible linker flags
3. run `dsltool` to convert ELF -> DSL

The helper function wraps this pipeline.

## Function reference

```cmake
ndsh_add_blocksds_dsl_library(
  TARGET <logical_name>
  SOURCES <src1> [src2...]
  [OUTPUT_NAME <artifact_base_name>]
  [INCLUDE_DIRECTORIES <dir1> [dir2...]]
  [COMPILE_DEFINITIONS <def1> [def2...]]
  [MAIN_ELF <path/to/main.elf>]
)
```

### Arguments

- `TARGET` (required)
  - Logical name for internal targets and output variables.
- `SOURCES` (required)
  - Source files for the dynamic library.
- `OUTPUT_NAME` (optional)
  - Basename for generated files in `build/`.
- `INCLUDE_DIRECTORIES` (optional)
  - Include dirs used only for this DSL.
- `COMPILE_DEFINITIONS` (optional)
  - Compile definitions used only for this DSL.
- `MAIN_ELF` (optional)
  - If set, passed as `dsltool -m <main.elf>` to resolve unknown symbols at build time.

## Returned variables

For `TARGET my_plugin`, the function sets:

- `${my_plugin_ELF}`: generated ELF path
- `${my_plugin_DSL}`: generated DSL path
- `${my_plugin_DSL_TARGET}`: custom target that builds that DSL

## Single DSL example

```cmake
ndsh_add_blocksds_dsl_library(
  TARGET ndsh_dylib_demo
  OUTPUT_NAME ndsh_dylib_demo
  SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/demo.cpp
)

add_custom_target(ndsh-dylib-demo ALL
  DEPENDS ${ndsh_dylib_demo_DSL_TARGET})
```

## Multiple DSL example

```cmake
ndsh_add_blocksds_dsl_library(
  TARGET plugin_math
  SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/math.cpp
)

ndsh_add_blocksds_dsl_library(
  TARGET plugin_time
  SOURCES ${CMAKE_SOURCE_DIR}/src/dylib_demo/time.cpp
)

add_custom_target(ndsh-dylibs ALL
  DEPENDS
    ${plugin_math_DSL_TARGET}
    ${plugin_time_DSL_TARGET}
)
```

## Symbol notes for dlsym()

- C++ symbols are mangled by default.
- Either:
  - use mangled names with `dlsym()`, or
  - export functions as `extern "C"` for stable symbol names.

## BlocksDS-only behavior

The helper module is a no-op unless `NDS_TOOLCHAIN_VENDOR` is `blocks`.
