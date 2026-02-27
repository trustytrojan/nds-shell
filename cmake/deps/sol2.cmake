setcb(SOL2_ENABLE_INSTALL OFF)
add_compile_definitions(SOL_EXCEPTIONS=0)
FetchContent_Declare(sol2 URL https://github.com/ThePhD/sol2/archive/v3.5.0.tar.gz)
FetchContent_MakeAvailable(sol2)

target_link_libraries(nds-shell PRIVATE sol2)
