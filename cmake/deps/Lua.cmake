setcb(LUA_SUPPORT_DL OFF)
setcb(LUA_ENABLE_SHARED OFF)
setcb(LUA_ENABLE_TESTING OFF)
setcb(LUA_BUILD_BINARY OFF)
setcb(LUA_BUILD_COMPILER OFF)
FetchContent_Declare(lua
	URL https://github.com/walterschell/Lua/archive/v5.4.7.tar.gz
	# enable 32-bit numbers. needs double-escaping as it passes through cmake's ExternalProject_Add()
	PATCH_COMMAND sed -i "s/#define LUA_32BITS\\\\s\\\\+0/#define LUA_32BITS 1/" "lua-5.4.7/include/luaconf.h"
)
FetchContent_MakeAvailable(lua)

target_link_libraries(nds-shell PRIVATE lua_static)
