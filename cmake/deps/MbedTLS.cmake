setcb(ENABLE_PROGRAMS OFF)
setcb(ENABLE_TESTING OFF)

add_compile_definitions(
	_POSIX_VERSION=202507 # just needs to be something greater than the 2000s
	MBEDTLS_TEST_SW_INET_PTON # libnds doesn't provide inet_pton()
)

FetchContent_Declare(mbedtls URL https://github.com/trustytrojan/mbedtls/releases/download/3.6.4-nds-r1/mbedtls-3.6.4-nds.7z)
FetchContent_MakeAvailable(mbedtls)

# list(APPEND CMAKE_PREFIX_PATH "${mbedtls_BINARY_DIR}/cmake")
set(MBEDTLS_INCLUDE_DIR "${mbedtls_SOURCE_DIR}/include" CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARIES "mbedx509;mbedcrypto;mbedtls" CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARY "mbedtls" CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY "mbedx509" CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY "mbedcrypto" CACHE STRING "" FORCE)

target_link_libraries(nds-shell PRIVATE mbedtls)
