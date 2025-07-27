# you MUST use the catnip cmake wrapper provided by devkitpro!

cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0135 NEW)
project(nds-shell LANGUAGES CXX)

set(CMAKE_SYSTEM_PROCESSOR "armv5te") # ARM9
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
include(Platform/NintendoDS)
include(FetchContent)

add_compile_options(-fno-exceptions)
add_compile_definitions(SOL_EXCEPTIONS=0)

set(LUA_SUPPORT_DL OFF)
set(LUA_ENABLE_SHARED OFF)
set(LUA_ENABLE_TESTING OFF)
set(LUA_BUILD_BINARY OFF)
set(LUA_BUILD_COMPILER OFF)
FetchContent_Declare(lua URL https://github.com/walterschell/Lua/archive/master.zip)
FetchContent_Declare(sol2 URL https://github.com/ThePhD/sol2/archive/develop.zip)
FetchContent_MakeAvailable(lua sol2)

# enable 32-bit numbers
execute_process(
	COMMAND sed -i "s/#define LUA_32BITS\\s\\+0/#define LUA_32BITS 1/" "${lua_SOURCE_DIR}/lua-5.4.7/include/luaconf.h"
	COMMAND_ERROR_IS_FATAL ANY
)

add_compile_options(-fno-rtti)
include_directories(include)
link_libraries(dswifi9 fat nds9 lua_static sol2)

file(GLOB_RECURSE SOURCES src/**)
add_executable(nds-shell ${SOURCES})
nds_create_rom(nds-shell)
