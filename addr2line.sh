#!/bin/sh
[ $1 ] || { exit 1; }
$DEVKITARM/bin/arm-none-eabi-addr2line -e build/main.release/nds-shell.elf "$1"