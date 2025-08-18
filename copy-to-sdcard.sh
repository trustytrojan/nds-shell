#!/bin/sh
# Quickly copy nds-shell and other files to your SD card.
sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 -m /mnt/sdcard
cp build/main.release/nds-shell.nds /mnt/sdcard
cp -r lua /mnt/sdcard # all lua test scripts
cp melonds-sdcard/lua/json.lua /mnt/sdcard/lua # rxi/json.lua
cp melonds-sdcard/.ndsh_history /mnt/sdcard # custom history file
cp melonds-sdcard/.ndshrc /mnt/sdcard # custom .ndshrc
sudo umount /mnt/sdcard
