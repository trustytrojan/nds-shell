# nds-shell
[![Build](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml)

A POSIX `sh`-like shell & environment for the Nintendo DS.

Click [here](https://nightly.link/trustytrojan/nds-shell/workflows/ci/main/nds-shell.zip) to get the latest GitHub Actions build. Check the [issues](https://github.com/trustytrojan/nds-shell/issues) tab to see all currently known issues. Report new issues there if you find any.

Join the [Discord server](https://discord.gg/YNSPCgPnAB)!

## Synopsis
Although it may seem like it, the scope of this project is *not* to be an operating system. That has been attempted several times before, not to mention the existence of [DSLinux](https://www.dslinux.org/), which has been long abandoned. Thanks to the [devkitPro](https://devkitpro.org) team, turning the DS into a general-purpose computer is a lot easier than it was back then. That's what this project aims to do: make the work of [libnds](https://github.com/devkitPro/libnds) fully interactive, while adding some fun features on top!

## Features
- `sh`-like features: cursor navigation, I/O redirection, environment variables, variable substitution, command history
  - Command history is now saved to your sdcard at `/.ndsh_history`!
  - Like `.bashrc` for Bash, `.ndshrc` at the root of your sdcard is automatically run upon shell startup
- Filesystem manipulation using typical POSIX shell commands: `ls`, `cd`, `cat`, etc
- Networking commands: `wifi`, `dns`, `tcp`, `curl`, `ssh`, `telnet`
- Lua interpreter, with an API to make your scripts feel just like builtin commands!
- HTTPS/SSL support with cURL and MbedTLS!
  - HUGE thanks to [this blogpost](https://git.vikingsoftware.com/blog/libcurl-with-mbedtls) for figuring out the CMake quirks

## Dependencies
- [libnds (my console rework fork)](https://github.com/trustytrojan/libnds/tree/console-rework), [dswifi](https://github.com/devkitPro/dswifi) and [libfat](https://github.com/devkitPro/libfat)
- [MbedTLS (my fork with CMake changes)](https://github.com/trustytrojan/mbedtls/tree/3.6.4-nds)
- [libcurl (my fork with CMake changes)](https://github.com/trustytrojan/curl/tree/8.15.0-mbedtls)
- [Lua (CMake compatible repo by @walterschell)](https://github.com/walterschell/Lua)
- [sol2](https://github.com/ThePhD/sol2)
- [libssh2 (my fork with CMake changes)](https://github.com/trustytrojan/libssh2/tree/1.11.1-nds)
- [libtelnet (my fork with CMake changes)](https://github.com/trustytrojan/libtelnet/tree/cmake-changes)

## Building
*note: this process has only been tested on linux*

1. Get [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started) on your system
2. Install the `nds-dev` metapackage (explained in the link above)
3. **Uninstall** `libnds` with `sudo (dkp-)pacman -Rdd libnds`
4. Clone, build, and install [my libnds fork](https://github.com/trustytrojan/libnds/tree/console-rework):
   ```sh
   git clone https://github.com/trustytrojan/libnds -b console-rework
   cd console-rework
   sudo -E make install -j$(nproc)
   ```
5. Clone this repo and run `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`
	- **Do NOT run CMake. You MUST run `catnip`.**

## To-do list (will be converted to issues)
- a way to push files to the DS remotely (try FTP with curl)
- use an argument parsing library for commands
