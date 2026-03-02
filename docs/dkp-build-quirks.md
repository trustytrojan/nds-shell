# devkitPro build quirks

## libcurl 8.18.0
-	you WILL ALWAYS get `CURLE_UNSUPPORTED_PROTOCOL` unless you set these cmake options:
	```cmake
	setcb(HAVE_FCNTL OFF)
	setcb(HAVE_FCNTL_H OFF)
	setcb(HAVE_FCNTL_O_NONBLOCK OFF)
	# this one is optional, but dkp/dswifi doesn't implement it!
	setcb(HAVE_SETSOCKOPT_SO_NONBLOCK OFF)
	setcb(HAVE_IOCTL_FIONBIO ON)
	```

	there are two reasons why:

	1.	https://github.com/curl/curl/blob/2eebc58c4b8d68c98c8344381a9f6df4cca838fd/lib/curlx/nonblock.c#L48
	
		devkitPro does not implement `fcntl()` anywhere along their DS code stack, and libcurl checks for it **first** before `ioctl()` which *is* implemented [here](https://github.com/devkitPro/dswifi/blob/ac8fbe9431d32cc4247df300d70c1688057da5e8/source/sgIP/sgIP_sockets.c#L360).
	
	2.	https://github.com/devkitPro/dswifi/blob/ac8fbe9431d32cc4247df300d70c1688057da5e8/source/sgIP/sgIP_sockets.c#L98

		devkitPro's dswifi `socket()` errors if `protocol` is anything but `0`.
