# nds-shell
[![Build](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml)

A POSIX `sh`-like shell & environment for the Nintendo DS.

Click [here](https://nightly.link/trustytrojan/nds-shell/workflows/ci/main/nds-shell.zip) to get the latest GitHub Actions build. Check the [issues](https://github.com/trustytrojan/nds-shell/issues) tab to see all currently known issues. Report new issues there if you find any.

Join the [Discord server](https://discord.gg/YNSPCgPnAB)!

## Synopsis
Although it may seem like it, the scope of this project is *not* to be an operating system. That has been attempted several times before, not to mention the existence of [DSLinux](https://www.dslinux.org/), which has been long abandoned. Thanks to the [devkitPro](https://devkitpro.org) team, turning the DS into a general-purpose computer is a lot easier than it was back then. That's what this project aims to do: make the work of [libnds](https://github.com/devkitPro/libnds) fully interactive, while adding some fun features on top!

## Features
- `sh`-like features: cursor navigation, I/O redirection, environment variables, variable substitution, command history
- Filesystem manipulation using typical POSIX-like commands: `ls`, `cd`, `cat`, etc
- Networking commands: `wifi`, `dns`, `tcp`, `curl`
- Hardware detection and optimization: DSi enhanced mode, GBA slot memory expansion
- System information commands: `hwinfo` to display hardware capabilities
- Lua interpreter (soon an API to add custom commands)
- HTTPS/SSL support with cURL and MbedTLS!
  - HUGE thanks to [this blogpost](https://git.vikingsoftware.com/blog/libcurl-with-mbedtls) for figuring out the CMake quirks

## Hardware Support
nds-shell automatically detects and optimizes for your hardware:

- **Nintendo DSi**: Enhanced mode with faster CPU and 16MB RAM (also works on 3DS)
- **Nintendo DS with GBA slot**: Memory expansion through GBA slot for additional 32MB
- **Nintendo DS**: Standard 4MB RAM mode

Use the `hwinfo` command to see detected hardware capabilities.

## Dependencies
- [libnds](https://github.com/devkitPro/libnds), which includes [dswifi](https://github.com/devkitPro/dswifi) and [libfat](https://github.com/devkitPro/libfat)
- [MbedTLS (my fork)](https://github.com/trustytrojan/mbedtls/tree/3.6.4-nds)
- [cURL (my fork)](https://github.com/trustytrojan/curl/tree/8.15.0-mbedtls)
- [Lua](https://lua.org)
- [sol2](https://github.com/ThePhD/sol2)

## Building
*note: this process has only been tested on linux*

1. Get [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started) on your system
2. Install the `nds-dev` metapackage (explained in the link above)
3. Run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`
	- **Do NOT run CMake. You must run `catnip`.**

## To-do list (will be converted to issues)
- a way to push files to the DS remotely (try FTP with curl)
- use an argument parsing library for commands
