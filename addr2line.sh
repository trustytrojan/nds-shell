#!/bin/sh
# Pass the `pc` address shown in a Guru Meditation error screen to convert it to a file:line number.
[ $1 ] || { echo 'address required'; exit 1; }
$DEVKITARM/bin/arm-none-eabi-addr2line -e build/main.release/nds-shell.elf $1