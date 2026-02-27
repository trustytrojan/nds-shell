echo "----------- patch command started here: $PWD"

# because devkitpro already defines this
sed -i '/#define socklen_t int/d' wolfssl/wolfcrypt/settings.h

# wolfssl thinks curl requires IPv6, but it doesn't.
# dkp does not support IPv6; manually remove the flag.
sed -i '/set(WOLFSSL_IP_ALT_NAME "yes")/d' CMakeLists.txt

# dkp doesn't provide inet_pton; we will provide our own
sed -i '/^#define WOLFSSL_IO_H/i extern int inet_pton(int af, const char *__restrict src, void *__restrict dst);' wolfssl/wolfio.h
