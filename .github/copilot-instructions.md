This is a C++ project implementing a POSIX-like shell & environment for the Nintendo DS. Currently we depend on devkitPro's devkitARM toolchain and libnds, dswifi, and libfat libraries. It also has Lua scripting support with a currently basic API for doing just what the built-in commands can do.

## Developer Environment Requirements
- `clang-format` for formatting, which is provided along with the clang toolchain.
- The devkitPro devkitARM toolchain, including dswifi, libfat, and libnds.
- See `README.md` and `.github/workflows/ci.yml` for instructions on how to get it setup

## Similar Projects
- https://github.com/lurk101/pshell
- https://www.lexaloffle.com/pico-8.php
- https://github.com/nesbox/TIC-80

## Code Standards

### Required Before Each Commit
- Run `./format.sh` before committing any changes to ensure proper code formatting. This **requires** that `clang-format` is installed, which is usually only provided along with the clang toolchain.

### Development Flow
- Build: `catnip -Tnds` (Read `CMakeLists.txt` for reference. Catnip is a CMake wrapper provided by devkitPro. Make sure it is installed in your environment before running this command.)
- Format: `./format.sh`

## Respository Structure
- `src`: sources
  - `commands`: commands that are larger than one small function are contains in their own source files here
  - `Commands.cpp`: the small simple commands that are implemented in one small function
  - `lua`: sol2/Lua binding code for nds-shell resources
- `include`: headers
- `lua`: lua testing scripts

## Key Guidelines
1. Follow C++ best practices and idiomatic patterns
2. Maintain existing code structure and organization
3. **Only** document public APIs and complex logic
