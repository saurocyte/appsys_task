#pragma once

#include <WinSock2.h>
#include <memory>
#include <string>

using addr_ptr = std::unique_ptr<addrinfo, void(__stdcall *)(addrinfo *)>;

struct addrinfo;

class General {
public:
	static void initialize_winsock();
	static addr_ptr resolve(std::string port, std::string addr);
};

