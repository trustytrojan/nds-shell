# nds-shell
[![Build](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml/badge.svg)](https://github.com/trustytrojan/nds-shell/actions/workflows/ci.yml)

A POSIX-like shell & environment for the Nintendo DS.

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
  - **NEW:** A web-`fetch()`-like API for Lua scripts! (for the goal of making a Discord client on nds-shell!)
  - *I didn't know about the [Pico-8](https://www.lexaloffle.com/pico-8.php) or the [TIC-80](https://tic80.com/) before... turns out I've unknowingly been creating just that, but **for real hardware!***
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

### Windows
1. Download the latest version of the [devkitPro installer](https://github.com/devkitPro/installer/releases) and select the `nds` option during installation
2. Open the `MSys2` shell from your start menu after installation and uninstall `libnds` with `pacman -Rdd libnds` 
*note: if you already have msys2 installed make sure you open the one that is exactly called `MSys2`, this is the one installed by devkitPro*

3. Clone, build and install [my libnds fork](https://github.com/trustytrojan/libnds/tree/console-rework):
```sh
git clone https://github.com/trustytrojan/libnds -b console-rework
cd libnds
make install -j$(nproc)
```

5. Clone this repo and build with `$DEVKITPRO/tools/bin/catnip -Tnds`

### Linux
1. Get [devkitPro pacman](https://devkitpro.org/wiki/Getting_Started) on your system
2. Install the `nds-dev` metapackage (explained in the link above)
3. **Uninstall** `libnds` with `sudo (dkp-)pacman -Rdd libnds`
4. Clone, build, and install [my libnds fork](https://github.com/trustytrojan/libnds/tree/console-rework):
   ```sh
   git clone https://github.com/trustytrojan/libnds -b console-rework
   cd libnds
   sudo -E make install -j$(nproc)
   ```
5. Clone this repo and build with `$DEVKITPRO/tools/bin/catnip -Tnds -j$(nproc)`

#### For both Windows and Linux remains:
- **Do NOT run CMake. You MUST run `catnip`.**

## Lua scripting notes
- For JSON support, I recommend [rxi/json.lua](https://github.com/rxi/json.lua). It's extremely lightweight.
- Remember that the `lua` command runs under a shell's thread. In your scripts **when performing work asynchronously, you should call `libnds.threadYield()`** (name subject to change) **to yield the shell thread running the `lua` command**.
  - The [Lua standard library for coroutines](https://www.lua.org/manual/5.4/manual.html#6.2) is available for use, however **Lua coroutines are NOT the same as system-level threads.** They all exist inside the Lua interpreter itself with no connection to the system thread running the `lua` command running your script.
