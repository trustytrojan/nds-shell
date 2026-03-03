#!/bin/sh
# Setup the SD card folder for melonDS, and also for copying to a real SD card
mkdir -p melonds-sdcard
ln -sf $PWD/lua melonds-sdcard
ln -sf $PWD/build/blocks/ndsh_dylib_demo.dsl melonds-sdcard
cd melonds-sdcard
cp /etc/ssl/cert.pem tls-ca-bundle.pem
ls json.lua || curl -LO https://github.com/rxi/json.lua/raw/refs/heads/master/json.lua