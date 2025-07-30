# nds-shell
A Bash-like shell for the Nintendo DS, using the [libnds](https://github.com/devkitPro/libnds) demo console/keyboard

## features
- lua interpreter (soon an API to add custom commands)
- networking (TCP/UDP)
- filesystem manipulation (typical unix shell commands: `ls`, `cd`, `cat`, etc)
- **new: HTTPS support with libcurl!!!!!!!!!!**

## building
*note: this process has only been tested on linux*

1. get [devkitpro](https://devkitpro.org/wiki/Getting_Started) on your system
2. install the `nds-dev` metapackage (explained in the link above)
3. run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`
	- **DO NOT RUN CMAKE. you MUST run catnip.**

## todo list
- a way to push files to the DS remotely (try FTP with curl)
- create a lua api for adding custom commands
