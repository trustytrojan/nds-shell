set -e
source .env
make
rm *.elf
desmume *.nds