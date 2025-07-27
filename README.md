# nds-shell
A Bash-like shell for the Nintendo DS, using the [libnds](https://github.com/devkitPro/libnds) demo console/keyboard

## features
- lua interpreter
- networking (tcp/udp)
- filesystem manipulation

## building
*note: the build process has only been tested on arch linux*

1. get [devkitpro](https://devkitpro.org/wiki/Getting_Started) on your system
2. install the `nds-dev` metapackage (explained in the link above)
3. run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)` (assuming `catnip` is not on your `PATH`).
	- **do NOT run cmake or any cmake ide/gui tool. you MUST run catnip.**
4. if you see errors about math functions, just change something in [CMakeLists.txt](./CMakeLists.txt) and rerun `catnip`

## todo list
- implement better http command with a proxy
- bind libnds functions to lua
