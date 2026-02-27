setcb(LIBTELNET_BUILD_UTIL OFF)
setcb(LIBTELNET_BUILD_DOC OFF)
setcb(LIBTELNET_BUILD_TEST OFF)
setcb(LIBTELNET_INSTALL OFF)
FetchContent_Declare(libtelnet URL https://github.com/trustytrojan/libtelnet/archive/cmake-changes.zip)
FetchContent_MakeAvailable(libtelnet)

target_link_libraries(nds-shell PRIVATE libtelnet)
