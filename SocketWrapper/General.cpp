#include "General.h"
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <memory>
#include <string>
#include "exceptions.h";

void General::initialize_winsock() {
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		throw SocketError("WinSock initialization failed");
	}
}

addr_ptr General::resolve(std::string port, std::string addr) {
	addrinfo hints = { 0 };
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* result = nullptr;
	int iResult = getaddrinfo(addr != "" ? addr.c_str() : NULL, port.c_str(), &hints, &result);
	addr_ptr u_result(result, &freeaddrinfo);
	if (iResult != 0) {
		throw SocketError("getaddrinfo failed");
	}

	return u_result;
}
