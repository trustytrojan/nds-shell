cmake_minimum_required(VERSION 3.21)

if(POLICY CMP0135)
	cmake_policy(SET CMP0135 NEW)
endif()

project(nds-shell LANGUAGES CXX)

macro(setcb var val)
	set(${var} ${val} CACHE BOOL "" FORCE)
endmacro()

set(CMAKE_SYSTEM_PROCESSOR "armv5te") # ARM9
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

include(FetchContent)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(insert_git_hash)

if(NDS_TOOLCHAIN_VENDOR STREQUAL "dkp")
	list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
	include(Platform/NintendoDS)
endif()

## options
option(NDSH_THREADING "Enable multithreading for multiconsole support" ON)
set(SSL_BACKEND "WolfSSL" CACHE STRING "SSL backend to use for libcurl/libssh2. Only \"WolfSSL\" and \"MbedTLS\" allowed.")
set_property(CACHE SSL_BACKEND PROPERTY STRINGS "WolfSSL" "MbedTLS")

file(GLOB_RECURSE SOURCES src/**)
add_executable(nds-shell ${SOURCES})

## dependencies (modules will link nds-shell to their libraries)
include(deps/${SSL_BACKEND})
include(deps/cURL)
include(deps/Lua)
include(deps/sol2)
include(deps/libssh2)
include(deps/libtelnet)

## nds-shell
add_compile_options(-fno-rtti -fno-exceptions) # this should be global for any C++ dependencies
target_include_directories(nds-shell PRIVATE include ${CMAKE_BINARY_DIR})

if(NDS_TOOLCHAIN_VENDOR STREQUAL "dkp")
	target_link_libraries(nds-shell PRIVATE dswifi9 fat) # libnds is linked by default
elseif(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	target_link_libraries(nds-shell PRIVATE dswifi9 fat)
endif()

## VERY important options
if(NDSH_THREADING)
	target_compile_definitions(nds-shell PRIVATE NDSH_THREADING)
	message("********************* THREADING IS ON  ********************")
else()
	message("********************* THREADING IS OFF ********************")
endif()

if(NOT CURL_DISABLE_VERBOSE_STRINGS)
	message("********************* CURL VERBOSE STRINGS ARE ON! THREADED HTTPS WILL CRASH!!! *********************")
	target_compile_definitions(nds-shell PRIVATE CURL_DEBUG)
endif()

## turn the ELF into an NDS file
nds_create_rom(nds-shell)
