#pragma once

#include <WinSock2.h>
#include <memory>

using addr_ptr = std::unique_ptr<addrinfo, void(__stdcall *)(addrinfo *)>;

struct addrinfo;

class General {
public:
	static int initialize_winsock();
	static addr_ptr resolve(PCSTR port, PCSTR addr);
};

