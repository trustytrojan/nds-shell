#!/bin/sh
# Copy everything from melonds-sdcard to a real SD card for running nds-shell on a real DS
sudo mount -o uid=1000,gid=1000 /dev/mmcblk0p1 -m /mnt/sdcard
cp build/nds-shell.nds /mnt/sdcard
cp -r lua /mnt/sdcard
cd melonds-sdcard
cp .ndsh_history /mnt/sdcard
cp .ndshrc /mnt/sdcard
cp tls-ca-bundle.pem /mnt/sdcard
cp ndsh_dylib_deno.dsl /mnt/sdcard
cp -r .ssh /mnt/sdcard
sudo umount /mnt/sdcard
