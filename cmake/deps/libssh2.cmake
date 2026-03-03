if(NDSH_SSL_BACKEND STREQUAL "MbedTLS")
	set(CRYPTO_BACKEND mbedTLS CACHE STRING "" FORCE)
elseif(NDSH_SSL_BACKEND STREQUAL "WolfSSL")
	set(CRYPTO_BACKEND wolfSSL CACHE STRING "" FORCE)
endif()

setcb(ENABLE_ZLIB_COMPRESSION OFF)
setcb(CLEAR_MEMORY OFF)
setcb(BUILD_EXAMPLES OFF)
setcb(BUILD_TESTING OFF)
FetchContent_Declare(libssh2
	URL https://github.com/trustytrojan/libssh2/archive/1.11.1-nds.tar.gz
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/.cmake_deps/libssh2-src
)
FetchContent_MakeAvailable(libssh2)

if(NDS_TOOLCHAIN_VENDOR STREQUAL "dkp")
	target_compile_definitions(libssh2_static PRIVATE _3DS)
endif()

if(NDSH_SSL_BACKEND STREQUAL "MbedTLS")
	target_compile_definitions(libssh2_static PRIVATE LIBSSH2_MBEDTLS)
endif()

target_link_libraries(nds-shell PRIVATE libssh2_static)
target_compile_definitions(nds-shell PRIVATE NDSH_LIBSSH2)
