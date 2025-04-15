# you MUST use the catnip cmake wrapper provided by devkitpro!

cmake_minimum_required(VERSION 3.13)
project(nds-shell LANGUAGES CXX)

set(CMAKE_SYSTEM_PROCESSOR "armv5te") # ARM9
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
include(Platform/NintendoDS)

include_directories(include)
link_libraries(dswifi9 fat nds9)

file(GLOB_RECURSE SOURCES src/**)
add_executable(nds-shell ${SOURCES})
nds_create_rom(nds-shell)
