#ifndef __EVERYTHING__
#define __EVERYTHING__

// libnds
#include <nds.h>
#include <dswifi9.h>

// stdc++
#include <iostream>
#include <sstream>
#include <vector>
#include <regex>
#include <set>
#include <unordered_map>

// networking
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

// dns
#include <netdb.h>

// functions
Wifi_AccessPoint *findAP(void);
sockaddr_in *parseIpAddress(const std::string &addr);
std::vector<std::string> strsplit(const std::string &str, const char delim);
void printWifiStatus(void);
void createTerminal(void);

#endif
