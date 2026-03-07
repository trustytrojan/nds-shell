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
message(STATUS "**** Selected SSL backend: '${NDSH_SSL_BACKEND}'")

## validation
if(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	message(AUTHOR_WARNING "BlocksDS toolchain detected. Disabling threading and multicon, as it's not ready yet.")
	setcb(NDSH_MULTICONSOLE OFF)
	setcb(NDSH_THREADING OFF)
endif()

if(NDSH_THREADING AND NOT NDSH_MULTICONSOLE)
	message(FATAL_ERROR "Multithreading with a single console is not supported.")
endif()

if(NDSH_SSH AND NOT NDSH_SSL_BACKEND)
	message(WARNING "SSH enabled without an SSL backend, expect build errors!")
endif()

## sources
set(NDSH_SOURCES
	# src/custom_swi_test.cpp
	src/main.cpp
	src/Shell.cpp
	src/CliPrompt.cpp
	src/Consoles.cpp
	src/Commands.cpp
	src/Lexer.cpp
	src/Parser.cpp
	src/NetUtils.cpp
	src/PrintVersions.cpp
	src/commands/tcp.cpp
	src/commands/wifi.cpp
	src/console/console-clear.cpp
	src/console/console-cursor.cpp
	src/console/console-esc.cpp
	src/console/console-print.cpp
)

if(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	list(APPEND NDSH_SOURCES src/commands/dylib.cpp)
endif()

if(NDSH_CURL)
	list(APPEND NDSH_SOURCES
		src/commands/curl.cpp
		src/commands/ws.cpp
		src/WebSocket.cpp
	)
	if(NDSH_THREADING)
		list(APPEND NDSH_SOURCES src/CurlMulti.cpp)
	endif()
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
endif()

include(deps/cURL)

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

function(target_link_undefined_symbols TARGET_NAME)
    set(LINK_OPTIONS "")
    
    # ARGN contains all arguments passed after TARGET_NAME
    foreach(SYMBOL IN LISTS ARGN)
        list(APPEND LINK_OPTIONS "-Wl,--undefined=${SYMBOL}")
    endforeach()

    # Apply the options to the target
    target_link_options(${TARGET_NAME} PRIVATE ${LINK_OPTIONS})
endfunction()

if(NDS_TOOLCHAIN_VENDOR STREQUAL "dkp")
	target_link_libraries(nds-shell PRIVATE dswifi9 fat) # libnds is linked by default
elseif(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	target_link_libraries(nds-shell PRIVATE dswifi9)

	# the below commented code should be appended curl-src/lib/CMakeLists.txt
	# annoyingly this will make every object build twice (since nds-shell links to it), but for now, i don't care
	# all that matters is... it works... see src/dylib_demo/libC.cpp

# # Include your DSL helper module
# include(BlocksDSDynamicLibraries)

# # Example: create a static library, then wrap it as a DSL.
# add_library(curl_dsl_static STATIC ${CSOURCES})
# ndsh_add_blocksds_dsl_library(
#     TARGET curl_dsl
#     STATIC_TARGET curl_dsl_static
#     OUTPUT_NAME curl
#     MAIN_TARGET nds-shell
#     DEPENDENCY_TARGETS 
#         # Add other DSLs curl might need (e.g., mbedtls or wolfssl)
#         ${SSL_DSL_TARGET}
# )

# # This forces the DSL to be built when you run 'make' or 'cmake --build'
# add_custom_target(build_curl_dsl ALL 
#     DEPENDS ${curl_dsl_DSL_TARGET}
# )


	target_link_undefined_symbols(nds-shell
		__aeabi_lmul
		accept
		getpeername
		strpbrk
		__aeabi_d2lz __ctzdi2 __popcountdi2 arc4random fcntl fgets getsockname getsockopt htonl qsort select setsockopt strcspn strrchr strspn
	)
endif()

## VERY important options
if(NDSH_THREADING)
	target_compile_definitions(nds-shell PRIVATE NDSH_THREADING)
	message("********************* THREADING IS ON  ********************")
else()
	message("********************* THREADING IS OFF ********************")
endif()

if(NDSH_CURL AND NOT CURL_DISABLE_VERBOSE_STRINGS)
	target_compile_definitions(nds-shell PRIVATE NDSH_CURL_DEBUG)
	message(WARNING "CURL VERBOSE STRINGS ARE ON! THREADED HTTPS WILL CRASH!!!")
endif()

if(NDSH_MULTICONSOLE)
	target_compile_definitions(nds-shell PRIVATE NDSH_MULTICONSOLE)
	message("********************* MULTICONSOLE IS ON  ********************")
else()
	message("********************* MULTICONSOLE IS OFF ********************")
endif()

## turn the ELF into an NDS file
nds_create_rom(nds-shell)

if(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	include(BlocksDSDylibDemo)
	ndsh_configure_blocksds_dylib_demo(MAIN_TARGET nds-shell)
endif()

## toolchain specific symlinks
# this helps clangd not be confused
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
		"${CMAKE_BINARY_DIR}/compile_commands.json"
		"${CMAKE_SOURCE_DIR}/build/compile_commands.json"
	COMMAND_ERROR_IS_FATAL ANY
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink 
		"${CMAKE_BINARY_DIR}/nds-shell.nds"
		"${CMAKE_SOURCE_DIR}/build/nds-shell.nds"
	COMMAND_ERROR_IS_FATAL ANY
)
