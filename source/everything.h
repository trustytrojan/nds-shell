#ifndef __EVERYTHING__
#define __EVERYTHING__

// libnds
#include <nds.h>
#include <dswifi9.h>

// stdc++
#include <iostream>
#include <sstream>
#include <vector>
#include <set>

// networking
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

// dns
#include <netdb.h>

// functions
Wifi_AccessPoint *findAP(void);
sockaddr_in *parseIpAddress(const std::string &addr);
sockaddr_in *parseAddress(const std::string &addr, const int defaultPort);
std::vector<std::string> strsplit(const std::string &str, const char delim);
void printWifiStatus(void);
void createTerminal(void);
void tcpClient(const int sock);
void resetKeyHandler(void);

// globals
extern Keyboard *keyboard;

#endif
