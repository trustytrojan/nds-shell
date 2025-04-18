# nds-shell
A Bash-like shell for the Nintendo DS, using the [libnds](https://github.com/devkitPro/libnds) demo console/keyboard

## features
- lua interpreter
- networking (tcp/udp)
- filesystem manipulation

## building
this process has only been tested on arch linux

1. get [devkitpro](https://devkitpro.org/wiki/Getting_Started) on your system
2. install the `nds-dev` metapackage (explained in the link above)
3. run `$DEVKITPRO/tools/bin/catnip -T nds`
4. if you see a crap ton of compile errors from lua, make sure to set this define to `1` in `luaconf.h`:
	```c
	/*
	@@ LUA_32BITS enables Lua with 32-bit integers and 32-bit floats.
	*/
	#define LUA_32BITS	1
	```
5. if you see errors about math functions being missing, just change something in [CMakeLists.txt](./CMakeLists.txt) and rerun `catnip`

## todo list
- implement tftp server
  - so we can push new shell binaries & lua scripts remotely!
- implement better http command with a proxy
- bind libnds functions to lua
