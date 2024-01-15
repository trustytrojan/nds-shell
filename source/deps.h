#ifndef __DEPS__
#define __DEPS__

// libnds
#include <nds.h>
#include <dswifi9.h>

// stdc++
#include <iostream>
#include <sstream>
#include <vector>

// networking
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

// dns
#include <netdb.h>

// functions
Wifi_AccessPoint *findAP(void);

#endif
