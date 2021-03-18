#include "pch.h"
#include "Socket.h"
#include <string>;
#include <WinSock2.h>
#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

int Socket::initialize_socket() {
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		std::cerr << "WSAStartup failed: " << iResult << std::endl;
		return -1;
	}

	addrinfo* result = nullptr, * ptr = nullptr, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, port, &hints, &result);
	if (iResult != 0) {
		std::cerr << "getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
		return -1;
	}

	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET) {
		std::cerr << "error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}

	iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}

	freeaddrinfo(result);
}

Socket::Socket(SocketType t, int MAX_CONNECTIONS=10) {
	switch (t) {
	case LISTENING:
		initialize_socket();
		if (listen(s, MAX_CONNECTIONS) == SOCKET_ERROR) {
			std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}
		reset_fdsets();
		break;
	case ESTABILISHED:
		break;
	}
}

bool ConnectionPool::is_ready() {
	return (select(NULL, &readfds, &writefds, &exceptfds, 0) > 0);
}

bool ConnectionPool::pending_conn_present() {
	return FD_ISSET(listening_socket, &readfds);
}

void ConnectionPool::accept() {
	if (pending_conn_present()) {
		SOCKET client_socket;
		client_socket = ::accept(listening_socket, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}
		clients.push_back(Connection(client_socket));
	}
}

bool ConnectionPool::is_readable(SOCKET socket) {
	 return FD_ISSET(&socket, &readfds);
}

void ConnectionPool::receive(std::function<void(char*, SOCKET)> dataHandler) {
	List::iterator it = clients.begin();
	while (it != clients.end()) {
		bool ok = true;
		if (is_readable(it->socket)) {
			int iResult = it->recieve();

			dataHandler(it->buffer + it->chars_in_buffer, it->socket);
			it->resetBuffer();

			if (iResult == 0) {
//				std::cout << timestamp() << " socket " << it->socket << " was closed by client" << std::endl;
				ok = false;
			}
			if (iResult == -1) {
//				std::cout << timestamp() << " something happenned to the socket "
//					<< it->socket << std::endl;
				ok = iResult == WSAEWOULDBLOCK;
			}
			else {
//				std::cout << timestamp() << " recieved \"" << command_name
//					<< "\" from " << it->socket << std::endl;
				it->chars_in_buffer += iResult;
			}

			FD_CLR(it->socket, &readfds);
		}

		if (FD_ISSET(it->socket, &writefds)) {}

		if (!ok) {
//			std::cout << timestamp() << " connection closed (" << it->socket << ")" << std::endl;
			closesocket(it->socket);
			clients.erase(it);
			it = clients.begin();
		}
		else {
			++it;
		}
	}
}

void ConnectionPool::reset() {
	List::iterator it = clients.begin();
	while (it != clients.end()) {
		FD_SET(it->socket, &readfds);
		FD_SET(it->socket, &writefds);
		++it;
	}
}

bool Socket::is_readable() {
	return FD_ISSET(s, &readfds);
}

bool Socket::is_writeable() {
	return FD_ISSET(s, &writefds);
}

bool Socket::is_error() {
	return FD_ISSET(s, &exceptfds);
}

Socket Socket::accept_connection() {
	Socket new_s = Socket(accept(s, NULL, NULL));
	if (new_s == INVALID_SOCKET) {
		std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return new_s;
	}
	return new_s;
}

void Socket::reset_fdsets() {
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(s, &readfds);
	FD_SET(s, &writefds);
	FD_SET(s, &exceptfds);
}

std::string get_ip_from_socket(const SOCKET socket) {
	sockaddr client_info = { 0 };
	int addrsize = sizeof(client_info);
	getsockname(socket, &client_info, &addrsize);
	char* ip = inet_ntoa(((sockaddr_in *) &client_info)->sin_addr);
	return std::string(ip);
}