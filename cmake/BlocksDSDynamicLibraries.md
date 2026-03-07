# BlocksDS Dynamic Libraries (DSL) in nds-shell

This project provides a reusable CMake helper in:

- `cmake/BlocksDSDynamicLibraries.cmake`

Use it to build one or more BlocksDS dynamic libraries (`.dsl`) from existing static library targets.

## Why a helper function exists

CMake `add_library()` does not allow arbitrary custom library types for project-built artifacts.
For DSLs, we need a custom pipeline:

1. build a static library target (`.a`)
2. link that archive into an ELF with DSL-compatible linker flags
3. run `dsltool` to convert ELF -> DSL

The helper function wraps this pipeline.

## Function reference

```cmake
ndsh_add_blocksds_dsl_library(
  TARGET <logical_name>
  STATIC_TARGET <existing_static_library_target>
  [OUTPUT_NAME <artifact_base_name>]
  [MAIN_TARGET <target_name>]
  [DEPENDENCY_TARGETS <target1> [target2...]]
)
```

### Arguments

- `TARGET` (required)
  - Logical name for internal targets and output variables.
- `STATIC_TARGET` (required)
  - Existing static library target used as linker input for this DSL.
- `OUTPUT_NAME` (optional)
  - Basename for generated files in `build/`.
- `MAIN_TARGET` (optional)
  - Uses `TARGET_FILE` of an existing CMake target for `dsltool -m`.
- `DEPENDENCY_TARGETS` (optional)
  - Each entry may be:
    - a CMake target name (its `TARGET_FILE` is passed to `dsltool -d`), or
    - another `ndsh_add_blocksds_dsl_library()` logical `TARGET` name (its generated `<TARGET>_ELF` is passed to `dsltool -d`).

## Returned variables

For `TARGET my_plugin`, the function sets:

- `${my_plugin_ELF}`: generated ELF path
- `${my_plugin_DSL}`: generated DSL path
- `${my_plugin_DSL_TARGET}`: custom target that builds that DSL

## Single DSL example

```cmake
ndsh_add_blocksds_dsl_library(
  TARGET ndsh_dylib_demo
  STATIC_TARGET ndsh_dylib_demo_static
  OUTPUT_NAME ndsh_dylib_demo
)

add_custom_target(ndsh-dylib-demo ALL
  DEPENDS ${ndsh_dylib_demo_DSL_TARGET})
```

## Multiple DSL example

```cmake
add_library(plugin_math_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/math.cpp)
add_library(plugin_time_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/time.cpp)

ndsh_add_blocksds_dsl_library(
  TARGET plugin_math
  STATIC_TARGET plugin_math_static
)

ndsh_add_blocksds_dsl_library(
  TARGET plugin_time
  STATIC_TARGET plugin_time_static
)

add_custom_target(ndsh-dylibs ALL
  DEPENDS
    ${plugin_math_DSL_TARGET}
    ${plugin_time_DSL_TARGET}
)
```

## Interdependency example (A depends on B and C)

```cmake
add_library(libB_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libB.cpp)
add_library(libC_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libC.cpp)
add_library(libA_static STATIC ${CMAKE_SOURCE_DIR}/src/dylib_demo/libA.cpp)

ndsh_add_blocksds_dsl_library(
  TARGET libB
  STATIC_TARGET libB_static
  MAIN_TARGET nds-shell
)

ndsh_add_blocksds_dsl_library(
  TARGET libC
  STATIC_TARGET libC_static
  MAIN_TARGET nds-shell
)

ndsh_add_blocksds_dsl_library(
  TARGET libA
  STATIC_TARGET libA_static
  MAIN_TARGET nds-shell
  DEPENDENCY_TARGETS
    libB
    libC
)
```

## Symbol notes for dlsym()

- C++ symbols are mangled by default.
- Either:
  - use mangled names with `dlsym()`, or
  - export functions as `extern "C"` for stable symbol names.

## BlocksDS-only behavior

The helper module is a no-op unless `NDS_TOOLCHAIN_VENDOR` is `blocks`.
