#!/usr/bin/env bash
set -euo pipefail
echo "----------- patch command started here: $PWD"

# because devkitpro already defines this
if grep -q '#define socklen_t int' wolfssl/wolfcrypt/settings.h; then
    sed -i '/#define socklen_t int/d' wolfssl/wolfcrypt/settings.h
else
    echo "wolfssl_patch.sh: expected '#define socklen_t int' in wolfssl/wolfcrypt/settings.h; not found" >&2
    exit 1
fi

# wolfssl thinks curl requires IPv6, but it doesn't.
# dkp does not support IPv6; manually remove the flag.
if grep -q 'set(WOLFSSL_IP_ALT_NAME "yes")' CMakeLists.txt; then
    sed -i '/set(WOLFSSL_IP_ALT_NAME "yes")/d' CMakeLists.txt
else
    echo "wolfssl_patch.sh: expected 'set(WOLFSSL_IP_ALT_NAME \"yes\")' in CMakeLists.txt; not found" >&2
    exit 1
fi

# dkp doesn't provide inet_pton; we will provide our own
if grep -q '^#define WOLFSSL_IO_H' wolfssl/wolfio.h; then
    sed -i '/^#define WOLFSSL_IO_H/i extern int inet_pton(int af, const char *__restrict src, void *__restrict dst);' wolfssl/wolfio.h
else
    echo "wolfssl_patch.sh: expected '#define WOLFSSL_IO_H' in wolfssl/wolfio.h; not found" >&2
    exit 1
fi
