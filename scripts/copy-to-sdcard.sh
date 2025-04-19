#!/bin/sh
sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 -m /mnt/sdcard
cp build/main.release/nds-shell.nds /mnt/sdcard
cp lua-scripts/{*,.*} /mnt/sdcard
sudo umount /dev/mmcblk0p1
