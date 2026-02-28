if(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	find_library(FIND_MBEDTLS mbedtls PATHS $ENV{BLOCKSDSEXT}/mbedtls/lib)
	find_library(FIND_MBEDX509 mbedx509 PATHS $ENV{BLOCKSDSEXT}/mbedtls/lib)
	find_library(FIND_MBEDCRYPTO mbedcrypto PATHS $ENV{BLOCKSDSEXT}/mbedtls/lib)
	message("******************** FIND_MBEDTLS: ${FIND_MBEDTLS}")
	message("******************** FIND_MBEDX509: ${FIND_MBEDX509}")
	message("******************** FIND_MBEDCRYPTO: ${FIND_MBEDCRYPTO}")
	if(NOT FIND_MBEDTLS MATCHES "-NOTFOUND" AND
	   NOT FIND_MBEDX509 MATCHES "-NOTFOUND" AND
	   NOT FIND_MBEDCRYPTO MATCHES "-NOTFOUND")
		set(MBEDTLS_INCLUDE_DIR "$ENV{BLOCKSDSEXT}/mbedtls/include" CACHE STRING "" FORCE)
		set(MBEDTLS_LIBRARIES "mbedx509;mbedcrypto;mbedtls" CACHE STRING "" FORCE)
		set(MBEDTLS_LIBRARY "mbedtls" CACHE STRING "" FORCE)
		set(MBEDX509_LIBRARY "mbedx509" CACHE STRING "" FORCE)
		set(MBEDCRYPTO_LIBRARY "mbedcrypto" CACHE STRING "" FORCE)
		target_link_libraries(nds-shell PRIVATE mbedtls)
		message(STATUS "***************** MbedTLS found! returning early")
		return()
	endif()
	message(WARNING "*********** BlocksDS in use but MbedTLS not found! building from source...")
endif()

setcb(ENABLE_PROGRAMS OFF)
setcb(ENABLE_TESTING OFF)

add_compile_definitions(
	_POSIX_VERSION=202507 # just needs to be something greater than the 2000s
	MBEDTLS_TEST_SW_INET_PTON # libnds doesn't provide inet_pton()
)

FetchContent_Declare(mbedtls URL https://github.com/trustytrojan/mbedtls/releases/download/3.6.4-nds-r1/mbedtls-3.6.4-nds.7z)
FetchContent_MakeAvailable(mbedtls)

set(MBEDTLS_INCLUDE_DIR "${mbedtls_SOURCE_DIR}/include" CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARIES "mbedx509;mbedcrypto;mbedtls" CACHE STRING "" FORCE)
set(MBEDTLS_LIBRARY "mbedtls" CACHE STRING "" FORCE)
set(MBEDX509_LIBRARY "mbedx509" CACHE STRING "" FORCE)
set(MBEDCRYPTO_LIBRARY "mbedcrypto" CACHE STRING "" FORCE)

target_link_libraries(nds-shell PRIVATE mbedtls)
