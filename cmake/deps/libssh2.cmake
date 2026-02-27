if(SSL_BACKEND STREQUAL "MbedTLS")
	setcb(CRYPTO_BACKEND mbedTLS)
elseif(SSL_BACKEND STREQUAL "WolfSSL")
	setcb(CRYPTO_BACKEND wolfSSL)
endif()

setcb(ENABLE_ZLIB_COMPRESSION OFF)
setcb(CLEAR_MEMORY OFF)
setcb(BUILD_EXAMPLES OFF)
setcb(BUILD_TESTING OFF)
# set(MBEDTLS_INCLUDE_DIR ${mbedtls_SOURCE_DIR}/include)
# set(MBEDCRYPTO_LIBRARY mbedcrypto)
FetchContent_Declare(libssh2 URL https://github.com/trustytrojan/libssh2/archive/1.11.1-nds.tar.gz)
FetchContent_MakeAvailable(libssh2)

target_compile_definitions(libssh2_static PRIVATE _3DS)

if(USE_MBEDTLS)
	target_compile_definitions(libssh2_static PRIVATE LIBSSH2_MBEDTLS)
endif()

target_link_libraries(nds-shell PRIVATE libssh2_static)
