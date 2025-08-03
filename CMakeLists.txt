# you MUST use the catnip cmake wrapper provided by devkitpro!
# DEVKITPRO & DEVKITARM env vars are expected to be set!

cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0135 NEW)
cmake_policy(SET CMP0077 NEW)
project(nds-shell LANGUAGES CXX)

set(CMAKE_SYSTEM_PROCESSOR "armv5te") # ARM9
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(BUILD_SHARED_LIBS OFF)

list(APPEND CMAKE_MODULE_PATH "$ENV{DEVKITPRO}/cmake")
include(Platform/NintendoDS)
include(FetchContent)

include(ProcessorCount)
ProcessorCount(CPU_COUNT)
if(CPU_COUNT EQUAL 0)
	set(CPU_COUNT 1)
endif()

## libnds (my fork)
FetchContent_Declare(libnds URL https://github.com/trustytrojan/libnds/archive/consoles-per-fd.zip)
FetchContent_MakeAvailable(libnds)
execute_process(
	COMMAND make -j${CPU_COUNT}
	WORKING_DIRECTORY ${libnds_SOURCE_DIR}
	COMMAND_ERROR_IS_FATAL ANY
)
include_directories(${libnds_SOURCE_DIR}/include)
link_directories(${libnds_SOURCE_DIR}/lib)

## mbedtls
set(ENABLE_PROGRAMS OFF)
set(ENABLE_TESTING OFF)
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
set(BUILD_LIBCURL_DOCS OFF)
set(BUILD_MISC_DOCS OFF)
set(BUILD_TESTING OFF)
set(BUILD_EXAMPLES OFF)
set(BUILD_CURL_EXE OFF)
set(BUILD_STATIC_LIBS ON)

set(CURL_ENABLE_EXPORT_TARGET OFF)
set(CURL_USE_LIBSSH2 OFF)
set(CURL_USE_LIBPSL OFF)
set(CURL_DISABLE_INSTALL ON)
set(CURL_DISABLE_BINDLOCAL ON)
set(CURL_DISABLE_SOCKETPAIR ON)

set(CURL_USE_MBEDTLS ON) # disables openssl by implication
set(MBEDTLS_LIBRARIES mbedx509 mbedcrypto mbedtls) # may not be necessary

set(ENABLE_CURL_MANUAL OFF)
set(ENABLE_UNIX_SOCKETS OFF)
set(ENABLE_IPV6 OFF)
set(ENABLE_THREADED_RESOLVER OFF)
set(ENABLE_ARES OFF)

set(HAVE_ATOMIC OFF)
set(HAVE_BASENAME OFF)

# nonblock
set(HAVE_FCNTL_O_NONBLOCK OFF)
set(HAVE_SETSOCKOPT_SO_NONBLOCK OFF)
set(HAVE_IOCTL_FIONBIO ON)

# socket api
set(HAVE_SOCKET ON)
set(HAVE_SELECT ON)
set(HAVE_RECV ON)
set(HAVE_SEND ON)
set(HAVE_CLOSESOCKET ON)
set(HAVE_GETSOCKNAME ON)
set(HAVE_GETPEERNAME ON)

set(HTTP_ONLY ON) # http(s), ws(s)
set(PICKY_COMPILER OFF) # -Wpedantic is annoying
set(USE_MANUAL OFF)

FetchContent_Declare(curl URL https://github.com/trustytrojan/curl/archive/8.15.0-mbedtls.zip)
FetchContent_MakeAvailable(curl)

## lua
set(LUA_SUPPORT_DL OFF)
set(LUA_ENABLE_SHARED OFF)
set(LUA_ENABLE_TESTING OFF)
set(LUA_BUILD_BINARY OFF)
set(LUA_BUILD_COMPILER OFF)
link_libraries(m) # has to be done manually for now
FetchContent_Declare(lua URL https://github.com/walterschell/Lua/archive/master.zip)
FetchContent_MakeAvailable(lua)

# enable 32-bit numbers
execute_process(
	COMMAND sed -i "s/#define LUA_32BITS\\s\\+0/#define LUA_32BITS 1/" "${lua_SOURCE_DIR}/lua-5.4.7/include/luaconf.h"
	COMMAND_ERROR_IS_FATAL ANY
)

## sol2
set(SOL2_ENABLE_INSTALL OFF)
FetchContent_Declare(sol2 URL https://github.com/ThePhD/sol2/archive/develop.zip)
FetchContent_MakeAvailable(sol2)

add_compile_options(-fno-rtti -fno-exceptions)
add_compile_definitions(SOL_EXCEPTIONS=0)
include_directories(include ${curl_SOURCE_DIR}/include)
link_libraries(dswifi9 fat lua_static sol2 libcurl)

file(GLOB_RECURSE SOURCES src/**)
add_executable(nds-shell ${SOURCES})
nds_create_rom(nds-shell)
