# nds-shell
A POSIX sh-like shell for the Nintendo DS

## Features
- Lua interpreter (soon an API to add custom commands)
- Filesystem manipulation (typical POSIX shell commands: `ls`, `cd`, `cat`, etc)
- **New: HTTPS support with libcurl!!!!!!!!!!**
  - HUGE thanks to https://git.vikingsoftware.com/blog/libcurl-with-mbedtls

## Dependencies
- [libnds](https://github.com/devkitPro/libnds)
- [MbedTLS (my fork)](https://github.com/trustytrojan/mbedtls/tree/3.6.4-nds)
- [cURL (my fork)](https://github.com/trustytrojan/curl/tree/8.15.0-mbedtls)
- [Lua](https://lua.org)
- [sol2](https://github.com/ThePhD/sol2)

## Building
*note: this process has only been tested on linux*

1. Get [devkitpro](https://devkitpro.org/wiki/Getting_Started) on your system
2. Install the `nds-dev` metapackage (explained in the link above)
3. Run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`
	- **Do NOT run CMake. You must run `catnip`.**

## To-do list (will be converted to issues)
- a way to push files to the DS remotely (try FTP with curl)
- add command to set curl's CA cert file instead of hardcoding
- use an argument parsing library for commands
