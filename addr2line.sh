#!/bin/sh
[ $1 ] || { echo 'address required'; exit 1; }
$DEVKITARM/bin/arm-none-eabi-addr2line -e build/dkp/nds-shell.elf $1
$WONDERFUL_TOOLCHAIN/toolchain/gcc-arm-none-eabi/bin/arm-none-eabi-addr2line -e build/blocks/nds-shell.elf $1