set -e
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPRO=/opt/devkitpro
make
rm *.elf
usermount /dev/mmcblk0p1 /mnt/sdcard
mv *.nds /mnt/sdcard
sudo umount /dev/mmcblk0p1