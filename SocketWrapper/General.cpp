#include "pch.h"
#include "General.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <memory>

int General::initialize_winsock() {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cerr << "WSAStartup failed: " << iResult << std::endl;
		return -1;
	}
	return 0;
}

addr_ptr General::resolve(PCSTR port, PCSTR addr=NULL) {
	addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* result = nullptr;
	int iResult = getaddrinfo(addr, port, &hints, &result);
	addr_ptr u_result(result, &freeaddrinfo);
	if (iResult != 0) {
		std::cerr << "getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
	}

	return u_result;
}
