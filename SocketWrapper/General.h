#pragma once

#include <WinSock2.h>

struct addrinfo;

class General {
public:
	static int initialize_winsock();
	static addrinfo* resolve(PCSTR port, PCSTR addr);
};

