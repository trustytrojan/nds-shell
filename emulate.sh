set -e
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPRO=/opt/devkitpro
make
rm *.elf
desmume *.nds