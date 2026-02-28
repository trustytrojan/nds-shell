setcb(WOLFSSL_EXAMPLES OFF)
setcb(WOLFSSL_INSTALL OFF)
setcb(WOLFSSL_CRYPT_TESTS OFF)
setcb(WOLFSSL_CURL ON)
setcb(WOLFSSL_IP_ALT_NAME OFF) # disable IPv6
setcb(WOLFSSL_QUIC ON)
setcb(WOLFSSL_SYS_CA_CERTS OFF)
setcb(WOLFSSL_TLS13 ON)
setcb(WOLFSSL_ECC ON)

FetchContent_Declare(wolfssl
	URL https://github.com/wolfSSL/wolfssl/archive/v5.8.4-stable.tar.gz
	PATCH_COMMAND bash "${CMAKE_SOURCE_DIR}/config/wolfssl_patch.sh"
)
FetchContent_MakeAvailable(wolfssl)

target_compile_definitions(wolfssl PRIVATE
	HAVE_SOCKADDR
	SOMAXCONN=1 # dkp doesn't define this; 1 is fine
	WOLFSSL_NO_ATOMICS
)

target_compile_definitions(wolfssl PUBLIC
	WOLFSSL_NDS # for libcurl
)

# we have to be the find_package(WolfSSL) because nobody else does it right
set(WOLFSSL_LIBRARY "wolfssl" CACHE STRING "" FORCE)
set(WOLFSSL_INCLUDE_DIR "${wolfssl_SOURCE_DIR}" CACHE STRING "" FORCE)

target_link_libraries(nds-shell PRIVATE wolfssl)
