#include "pch.h"
#include "Socket.h"
#include <string>;
#include <WinSock2.h>
#include <iostream>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "timestamp/timestamp.h"
#include "General.h"

ConnectionPool::ConnectionPool(unsigned int MAX_CONNECTIONS) 
: listening_socket(MAX_CONNECTIONS, PORT) {}

std::string ConnectionPool::ip() {
	return listening_socket.ip();
}

ClientSocket::ClientSocket(PCSTR addr, PCSTR port) {
    General::initialize_winsock();

    addrinfo *result = General::resolve(port, addr);

    // Attempt to connect to an address until one succeeds
    for(addrinfo *ptr=result; ptr != NULL; ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return;
        }

        // Connect to server.
        int iResult = connect(s, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (s == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return;
    }
}

ServerSocket::ServerSocket(int MAX_CONNECTIONS=10, PCSTR _port="") 
: port(_port) {
	General::initialize_winsock();

	addrinfo* result = General::resolve(port, NULL);

	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET) {
		std::cerr << "error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
	}

	int iResult = bind(s, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
	}

	freeaddrinfo(result);

	if (listen(s, MAX_CONNECTIONS) == SOCKET_ERROR) {
		std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
	}
}

bool ConnectionPool::is_ready() {
	return (select(NULL, &readfds, &writefds, &exceptfds, 0) > 0);
}

bool ConnectionPool::pending_conn_present() {
	return FD_ISSET(listening_socket.s, &readfds);
}

void ConnectionPool::accept() {
	if (pending_conn_present()) {
		SOCKET client_socket;
		client_socket = ::accept(listening_socket.s, NULL, NULL);
		if (client_socket == INVALID_SOCKET) {
			std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return;
		}
		std::cout << Timestamp::timestamp() << " new connection initiated (" << client_socket << ")" << std::endl;
		clients.push_back(Connection(client_socket));
	}
}

bool ConnectionPool::is_readable(SOCKET socket) {
	 return FD_ISSET(socket, &readfds);
}

void ConnectionPool::receive(std::function<void(char*, SOCKET)> dataHandler) {
	List::iterator it = clients.begin();
	while (it != clients.end()) {
		bool ok = true;
		if (is_readable(it->socket)) {
			int iResult = it->recieve();

			if (iResult == 0) {
				std::cout << Timestamp::timestamp() << " socket " << it->socket 
					<< " was closed by client" << std::endl;
				ok = false;
			}
			if (iResult == -1) {
				std::cout << Timestamp::timestamp() << " something happenned to the socket "
					<< it->socket << std::endl;
				ok = iResult == WSAEWOULDBLOCK;
			}
			else {
				dataHandler(it->buffer + it->chars_in_buffer, it->socket);
				it->chars_in_buffer += iResult;
				it->resetBuffer();
			}

			FD_CLR(it->socket, &readfds);
		}

		if (FD_ISSET(it->socket, &writefds)) {}

		if (!ok) {
			std::cout << Timestamp::timestamp() << " connection closed (" << it->socket << ")" << std::endl;
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
	FD_SET(listening_socket.s, &readfds);
}

std::string ServerSocket::ip() {
	sockaddr client_info = { 0 };
	int addrsize = sizeof(client_info);
	getsockname(s, &client_info, &addrsize);
	char* ip = inet_ntoa(((sockaddr_in *) &client_info)->sin_addr);
	return std::string(ip);
}
