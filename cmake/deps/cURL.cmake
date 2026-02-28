if(NDS_TOOLCHAIN_VENDOR STREQUAL "blocks")
	find_package(CURL)
	if(CURL_FOUND)
		message(STATUS "***************** CURL found! linking and returning early")
		target_link_libraries(nds-shell PRIVATE CURL::libcurl)
		return()
	endif()
endif()

# general configs
setcb(BUILD_LIBCURL_DOCS OFF)
setcb(BUILD_MISC_DOCS OFF)
setcb(BUILD_TESTING OFF)
setcb(BUILD_EXAMPLES OFF)
setcb(BUILD_CURL_EXE OFF)
setcb(BUILD_STATIC_LIBS ON) # !!!
setcb(CURL_ENABLE_EXPORT_TARGET OFF)
setcb(CURL_USE_LIBSSH2 OFF)
setcb(CURL_USE_LIBPSL OFF)
setcb(CURL_DISABLE_INSTALL ON)
setcb(CURL_DISABLE_BINDLOCAL ON)
setcb(CURL_DISABLE_SOCKETPAIR ON)

## THIS SETTING IS THE DECIDING FACTOR FOR WHETHER HTTPS WILL WORK IN THREADS!!!!!!!
# for some reason allowing verbose strings (which allows for debugging curl code)
# uses too much thread-local storage???? https://github.com/devkitPro/calico/pull/6
# better off just disabling this than breaking my head over it.
setcb(CURL_DISABLE_VERBOSE_STRINGS ON)

if(SSL_BACKEND STREQUAL "MbedTLS")
	setcb(CURL_USE_MBEDTLS ON)
	setcb(CURL_USE_WOLFSSL OFF)
elseif(SSL_BACKEND STREQUAL "WolfSSL")
	setcb(CURL_USE_MBEDTLS OFF)
	setcb(CURL_USE_WOLFSSL ON)
endif()

setcb(ENABLE_CURL_MANUAL OFF)
setcb(ENABLE_UNIX_SOCKETS OFF)
setcb(ENABLE_IPV6 OFF) # dkp does not support ipv6
setcb(ENABLE_THREADED_RESOLVER OFF)
setcb(ENABLE_ARES OFF)

setcb(HAVE_ATOMIC OFF)
setcb(HAVE_BASENAME OFF)

# nonblock
setcb(HAVE_FCNTL OFF)
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

FetchContent_Declare(curl URL https://github.com/curl/curl/archive/curl-8_18_0.tar.gz)
FetchContent_MakeAvailable(curl)

if(SSL_BACKEND STREQUAL "WolfSSL")
	# curl's cmake can't detect wolfSSL_BIO_new(), so we have to do this
	target_compile_definitions(libcurl_static PRIVATE HAVE_WOLFSSL_BIO_NEW)
endif()

target_link_libraries(nds-shell PRIVATE CURL::libcurl_static)
