set -e
export DEVKITARM=/opt/devkitpro/devkitARM
export DEVKITPRO=/opt/devkitpro
clear
make
sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 /mnt/sdcard
cp *.nds /mnt/sdcard
sudo umount /dev/mmcblk0p1