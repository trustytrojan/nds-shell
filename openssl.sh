#!/bin/sh
cd build/main.release/_deps/openssl-src

# configure
CFLAGS="-I$DEVKITPRO/libnds/include -DRIO_NOTIFIER_METHOD=1 -DOPENSSL_NO_UI_CONSOLE -DNO_SYSLOG -DOPENSSL_USE_IPV6=0" ./Configure no-apps no-tests gcc --cross-compile-prefix=$DEVKITPRO/devkitARM/bin/arm-none-eabi- --prefix=../openssl-build

# build and install
make install -j8

# you will need to change some things in the source code and in libnds headers
# but eventually it builds
