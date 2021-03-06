#include "Socket.h"
#include <string>;
#include <WinSock2.h>
#include <iostream>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include "timestamp/timestamp.h"
#include "General.h"
#include "exceptions.h"

int Connection::receive() {
	int iResult = recv(socket, buffer.data(), BUFLEN, 0);
	return iResult;
}

void Connection::reset_buffer() {
	buffer[0] = '\0';
}

ConnectionPool::ConnectionPool(unsigned int MAX_CONNECTIONS) 
: listening_socket(MAX_CONNECTIONS, PORT) {}

std::string ConnectionPool::ip() const {
	return listening_socket.ip();
}

std::string ConnectionPool::port() const {
	return PORT;
}


ClientSocket::ClientSocket(std::string addr, std::string port) {
    General::initialize_winsock();

    // Attempt to connect to an address until one succeeds
    for(addr_ptr ptr = General::resolve(port, addr); ptr != NULL; ptr.reset(ptr->ai_next)) {
        // Create a SOCKET for connecting to server
        s = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (s == INVALID_SOCKET) {
            throw SocketError("socket failed with error: " + WSAGetLastError());
        }

        // Connect to server.
        if (connect(s, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR) {
            closesocket(s);
            s = INVALID_SOCKET;
            continue;
        }
        break;
    }

    if (s == INVALID_SOCKET) {
		throw SocketError("unable to connect to the server!");
    }
}

ServerSocket::ServerSocket(int MAX_CONNECTIONS=10, std::string _port="") 
: port(_port) {
	General::initialize_winsock();

	addr_ptr result = General::resolve(port, "");

	s = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (s == INVALID_SOCKET) {
		throw SocketError("error at socket(): " + WSAGetLastError());
	}

	if (bind(s, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
		throw SocketError("bind() failed with error: " + WSAGetLastError());
	}

	if (listen(s, MAX_CONNECTIONS) == SOCKET_ERROR) {
		throw SocketError("listen failed with error: " + WSAGetLastError());
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

void ConnectionPool::receive(std::function<void(Connection::buffer_t, SOCKET)> dataHandler) {
	List::iterator it = clients.begin();
	while (it != clients.end()) {
		bool ok = true;
		if (is_readable(it->socket)) {
			int iResult = it->receive();

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
				dataHandler(it->buffer, it->socket);
				it->reset_buffer();
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

std::string ServerSocket::ip() const {
	sockaddr client_info = { 0 };
	int addrsize = sizeof(client_info);
	getsockname(s, &client_info, &addrsize);
	std::string ip = std::string(inet_ntoa(((sockaddr_in *) &client_info)->sin_addr));
	return std::string(ip);
}
