# nds-shell
A Bash-like shell for the Nintendo DS, using the [libnds](https://github.com/devkitPro/libnds) demo console/keyboard

## features
- lua interpreter (soon an API to add custom commands)
- networking (TCP/UDP)
- filesystem manipulation (typical unix shell commands: `ls`, `cd`, `cat`, etc)
- **new: HTTPS support with libcurl!!!!!!!!!!**
  - HUGE thanks to https://git.vikingsoftware.com/blog/libcurl-with-mbedtls

## dependencies
- [libnds](https://github.com/devkitPro/libnds)
- [mbedtls (my fork)](https://github.com/trustytrojan/mbedtls/tree/3.6.4-nds)
- [curl (my fork)](https://github.com/trustytrojan/curl/tree/8.15.0-mbedtls)
- [lua](https://lua.org)
- [sol2](https://github.com/ThePhD/sol2)

## building
*note: this process has only been tested on linux*

1. get [devkitpro](https://devkitpro.org/wiki/Getting_Started) on your system
2. install the `nds-dev` metapackage (explained in the link above)
3. run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`
	- **do NOT run cmake. you must run catnip.**

## todo list
- a way to push files to the DS remotely (try FTP with curl)
- create a lua api for adding custom commands
- add command to set curl's CA cert file instead of hardcoding
- use proper shell/argument parsing library for commands
  - or use the existing lexer/parser code in `unused-src`, but this is a waste of time if a bash parser library exists
