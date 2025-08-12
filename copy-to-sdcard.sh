#!/bin/sh
# Quickly copy nds-shell and other files to your SD card.
sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 -m /mnt/sdcard
cp build/main.release/nds-shell.nds /mnt/sdcard
sudo umount /mnt/sdcard
