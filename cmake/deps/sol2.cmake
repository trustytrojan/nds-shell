setcb(SOL2_ENABLE_INSTALL OFF)
FetchContent_Declare(sol2
	URL https://github.com/ThePhD/sol2/archive/v3.5.0.tar.gz
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/.cmake_deps/sol2-src
)
FetchContent_MakeAvailable(sol2)

target_link_libraries(nds-shell PRIVATE sol2)
target_compile_definitions(nds-shell PRIVATE
	NDSH_SOL2
	SOL_NO_EXCEPTIONS=1 # VERY IMPORTANT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
)
