# you MUST use the catnip cmake wrapper provided by devkitpro!
# DEVKITPRO & DEVKITARM env vars are expected to be set!

cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0077 NEW)
project(nds-shell LANGUAGES CXX)

set(CMAKE_SYSTEM_PROCESSOR "armv5te") # ARM9
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(BUILD_SHARED_LIBS OFF)

option(NDSH_THREADING "Enable multithreading for multiconsole support" ON)

macro(setcb var val)
	set(${var} ${val} CACHE BOOL "" FORCE)
endmacro()

list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
include(Platform/NintendoDS)
include(FetchContent)

## mbedtls
setcb(ENABLE_PROGRAMS OFF)
setcb(ENABLE_TESTING OFF)
add_compile_definitions(
	_POSIX_VERSION=202507 # just needs to be something greater than the 2000s
	MBEDTLS_TEST_SW_INET_PTON # libnds doesn't provide inet_pton()
)
FetchContent_Declare(mbedtls
	GIT_REPOSITORY https://github.com/trustytrojan/mbedtls
	GIT_TAG 3.6.4-nds)
FetchContent_MakeAvailable(mbedtls)
list(APPEND CMAKE_PREFIX_PATH "${mbedtls_BINARY_DIR}/cmake")

## curl
setcb(BUILD_LIBCURL_DOCS OFF)
setcb(BUILD_MISC_DOCS OFF)
setcb(BUILD_TESTING OFF)
setcb(BUILD_EXAMPLES OFF)
setcb(BUILD_CURL_EXE OFF)
setcb(BUILD_STATIC_LIBS ON)

setcb(CURL_ENABLE_EXPORT_TARGET OFF)
setcb(CURL_USE_LIBSSH2 OFF)
setcb(CURL_USE_LIBPSL OFF)
setcb(CURL_DISABLE_INSTALL ON)
setcb(CURL_DISABLE_BINDLOCAL ON)
setcb(CURL_DISABLE_SOCKETPAIR ON)

## THIS SETTING IS THE DECIDING FACTOR FOR WHETHER HTTPS WILL WORK IN THREADS!!!!!!!
# for some reason allowing verbose strings (which allows for debugging curl code)
# uses too much thread-local storage???? https://github.com/devkitPro/calico/pull/6
# the culprit could also be mbedtls; still better off just disabling this than breaking my head over it.
setcb(CURL_DISABLE_VERBOSE_STRINGS ON)

setcb(CURL_USE_MBEDTLS ON)
set(MBEDTLS_LIBRARIES mbedx509 mbedcrypto mbedtls)

setcb(ENABLE_CURL_MANUAL OFF)
setcb(ENABLE_UNIX_SOCKETS OFF)
setcb(ENABLE_IPV6 OFF)
setcb(ENABLE_THREADED_RESOLVER OFF)
setcb(ENABLE_ARES OFF)

setcb(HAVE_ATOMIC OFF)
setcb(HAVE_BASENAME OFF)

# nonblock
setcb(HAVE_FCNTL_O_NONBLOCK OFF)
setcb(HAVE_SETSOCKOPT_SO_NONBLOCK OFF)
setcb(HAVE_IOCTL_FIONBIO ON)

# socket api
setcb(HAVE_SOCKET ON)
setcb(HAVE_SELECT ON)
setcb(HAVE_RECV ON)
setcb(HAVE_SEND ON)
setcb(HAVE_CLOSESOCKET ON)
setcb(HAVE_GETSOCKNAME ON)
setcb(HAVE_GETPEERNAME ON)

setcb(HTTP_ONLY ON) # http(s), ws(s)
setcb(PICKY_COMPILER OFF) # -Wpedantic is annoying
setcb(USE_MANUAL OFF)

FetchContent_Declare(curl URL https://github.com/trustytrojan/curl/archive/8.15.0-mbedtls.zip)
FetchContent_MakeAvailable(curl)

## lua
setcb(LUA_SUPPORT_DL OFF)
setcb(LUA_ENABLE_SHARED OFF)
setcb(LUA_ENABLE_TESTING OFF)
setcb(LUA_BUILD_BINARY OFF)
setcb(LUA_BUILD_COMPILER OFF)
link_libraries(m) # has to be done manually for now
FetchContent_Declare(lua URL https://github.com/walterschell/Lua/archive/master.zip)
FetchContent_MakeAvailable(lua)

# enable 32-bit numbers
execute_process(
	COMMAND sed -i "s/#define LUA_32BITS\\s\\+0/#define LUA_32BITS 1/" "${lua_SOURCE_DIR}/lua-5.4.7/include/luaconf.h"
	COMMAND_ERROR_IS_FATAL ANY
)

## sol2
setcb(SOL2_ENABLE_INSTALL OFF)
add_compile_definitions(SOL_EXCEPTIONS=0)
FetchContent_Declare(sol2 URL https://github.com/ThePhD/sol2/archive/develop.zip)
FetchContent_MakeAvailable(sol2)

## libssh2
setcb(CRYPTO_BACKEND mbedTLS)
setcb(ENABLE_ZLIB_COMPRESSION OFF)
setcb(CLEAR_MEMORY OFF)
setcb(BUILD_EXAMPLES OFF)
setcb(BUILD_TESTING OFF)
set(MBEDTLS_INCLUDE_DIR ${mbedtls_SOURCE_DIR}/include)
set(MBEDCRYPTO_LIBRARY mbedcrypto)
add_compile_definitions(LIBSSH2_MBEDTLS _3DS)
FetchContent_Declare(libssh2 URL https://github.com/trustytrojan/libssh2/archive/1.11.1-nds.zip)
FetchContent_MakeAvailable(libssh2)

## libtelnet
FetchContent_Declare(telnet URL https://github.com/EmergReanimator/libtelnet/archive/develop.zip)
FetchContent_MakeAvailable(telnet)

## nds-shell
add_compile_options(-fno-rtti -fno-exceptions)
include_directories(include)
link_libraries(dswifi9 fat lua_static sol2 libcurl libssh2 libtelnet)

if(NDSH_THREADING)
	add_compile_definitions(NDSH_THREADING)
	message("********************* THREADING IS ON *********************")
else()
	message("********************* THREADING IS OFF *********************")
endif()

if(NOT CURL_DISABLE_VERBOSE_STRINGS)
	message("********************* CURL VERBOSE STRINGS ARE ON! THREADED HTTPS WILL CRASH!!! *********************")
	add_compile_definitions(CURL_DEBUG)
endif()

file(GLOB_RECURSE SOURCES src/**)
add_executable(nds-shell ${SOURCES})
nds_create_rom(nds-shell)
