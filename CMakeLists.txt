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
option(NDSH_MULTICONSOLE "Enable multiconsole" ON)
option(NDSH_THREADING "Enable multithreading" ON)
option(NDSH_CURL "Enable libcurl for HTTP(S) and WebSocket" ON)
option(NDSH_SSH "Enable SSH" ON)
option(NDSH_TELNET "Enable TELNET" ON)
option(NDSH_LUA "Enable Lua support with sol2" ON)

set(NDSH_SSL_BACKEND "WolfSSL" CACHE STRING "SSL backend to use for libcurl/libssh2. May be empty.")
set_property(CACHE NDSH_SSL_BACKEND PROPERTY STRINGS "WolfSSL" "MbedTLS")

## validation
if(NDSH_THREADING AND NOT NDSH_MULTICONSOLE)
	message(FATAL_ERROR "Multithreading with a single console is not supported.")
endif()

## sources
set(NDSH_SOURCES
	src/main.cpp
	src/Shell.cpp
	src/CliPrompt.cpp
	src/Consoles.cpp
	src/Commands.cpp
	src/Lexer.cpp
	src/Parser.cpp
	src/NetUtils.cpp
	src/commands/tcp.cpp
	src/commands/wifi.cpp
)

if(NDSH_CURL)
	list(APPEND NDSH_SOURCES
		src/commands/curl.cpp
		src/commands/ws.cpp
		src/CurlMulti.cpp
		src/WebSocket.cpp
	)
endif()

if(NDSH_LUA)
	list(APPEND NDSH_SOURCES
		src/lua/load_CliPrompt.cpp
		src/lua/load_CommandContext.cpp
		src/lua/load_libnds_funcs.cpp
		src/lua/load_Shell.cpp
		src/lua/my_state.cpp
		src/commands/lua.cpp
	)
	if(NDSH_CURL)
		list(APPEND NDSH_SOURCES
			src/lua/load_fetch.cpp
			src/lua/load_WebSocket.cpp
		)
	endif()
endif()

if(NDSH_SSH)
	list(APPEND NDSH_SOURCES
		src/commands/ssh.cpp
	)
endif()

if(NDSH_TELNET)
	list(APPEND NDSH_SOURCES
		src/commands/telnet.cpp
	)
endif()

add_executable(nds-shell ${NDSH_SOURCES})

## dependencies
if(NDSH_SSL_BACKEND)
	include(deps/${NDSH_SSL_BACKEND})
endif()

if(NDSH_CURL)
	include(deps/cURL)
endif()

if(NDSH_LUA)
	include(deps/Lua)
	include(deps/sol2)
endif()

if(NDSH_SSH)
	include(deps/libssh2)
endif()

if(NDSH_TELNET)
	include(deps/libtelnet)
endif()

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

if(NDSH_CURL AND NOT CURL_DISABLE_VERBOSE_STRINGS)
	target_compile_definitions(nds-shell PRIVATE CURL_DEBUG)
	message("********************* CURL VERBOSE STRINGS ARE ON! THREADED HTTPS WILL CRASH!!! *********************")
endif()

if(NDSH_MULTICONSOLE)
	target_compile_definitions(nds-shell PRIVATE NDSH_MULTICONSOLE)
	message("********************* MULTICONSOLE IS ON  ********************")
else()
	message("********************* MULTICONSOLE IS OFF ********************")
endif()

## turn the ELF into an NDS file
nds_create_rom(nds-shell)
