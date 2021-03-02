#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <map>
#include <iterator>
#include <chrono>
#include <ctime>
#include <iomanip>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "SafeQueue.h"
#include "../tinyxml2.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT "27015"
#define MAX_CONNECTIONS 10
#define BUFLEN 512

using namespace tinyxml2;
	
struct Connection {
	SOCKET socket;
	char buffer[BUFLEN];
	u_int chars_in_buffer;

	void resetBuffer() {
		buffer[0] = '\0';
		chars_in_buffer = 0;
	}

	Connection(SOCKET _socket) : 
		socket(_socket),
		buffer{ 0 },
		chars_in_buffer(0) {};
};

struct Request {
	std::string command;
	SOCKET *initiator;

	Request(std::string _command, SOCKET* _initiator) :
		command(_command),
		initiator(_initiator) 
	{};
};

struct Response {
	std::string response;
	SOCKET *destination;

	Response(std::string _response, SOCKET* _destination) :
		response(_response),
		destination(_destination) 
	{};
};

typedef std::vector<Connection> ConnectionList;
typedef SafeQueue<Request> RequestQueue;
typedef SafeQueue<Response> ResponseQueue;
typedef char Int8;
typedef short int Int16;
typedef int Int32;

auto timestamp() {
	time_t now = time(nullptr) ;
	return std::put_time(localtime(&now), "%T");
}

int initialize_listen_socket(SOCKET &listen_socket) {
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

	iResult = getaddrinfo(NULL, PORT, &hints, &result);
	if (iResult != 0) {
		std::cerr << "getaddrinfo failed: " << iResult << std::endl;
		WSACleanup();
		return -1;
	}

	listen_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listen_socket == INVALID_SOCKET) {
		std::cerr << "error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}

	iResult = bind(listen_socket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cerr << "bind() failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return -1;
	}

	freeaddrinfo(result);
}

void initialize_fdsets(fd_set& readfds, fd_set& writefds, fd_set& exceptfds, SOCKET &listen_socket) {
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&exceptfds);
	FD_SET(listen_socket, &readfds);
}

std::string get_ip_from_socket(const SOCKET socket) {
	sockaddr client_info = { 0 };
	int addrsize = sizeof(client_info);
	getsockname(socket, &client_info, &addrsize);
	char* ip = inet_ntoa(((sockaddr_in *) &client_info)->sin_addr);
	return std::string(ip);
}

std::string parse_buffer(char* buffer) {
	Int16 name_size = buffer[0];
	name_size = name_size << 8;
	name_size |= buffer[1];

	std::string command_name = "";
	for (int i = 0; i < name_size; ++i) {
		command_name += buffer[2 + i];
	}

	return command_name;
}

void worker_thread(RequestQueue &qr, ResponseQueue &qp) {
	SOCKET listen_socket;
	initialize_listen_socket(listen_socket);

	if (listen(listen_socket, MAX_CONNECTIONS) == SOCKET_ERROR) {
		std::cerr << "listen failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}

	fd_set readfds, writefds, exceptfds;
	ConnectionList client_sockets;

	const std::string ip = get_ip_from_socket(listen_socket);
	std::cout << timestamp() << " listening on " << ip << ":" << PORT << std::endl;

	while (true) {
		initialize_fdsets(readfds, writefds, exceptfds, listen_socket);

		ConnectionList::iterator it = client_sockets.begin();
		while (it != client_sockets.end()) {
			FD_SET(it->socket, &readfds);
			FD_SET(it->socket, &writefds);
			++it;
		}

		// Respond to pending connections
		while (!qp.empty()) {
			Response r = qp.pop();
			int iResult = send(*r.destination, r.response.c_str(), sizeof(r.response), 0);
			std::cout << timestamp() << " sent \"" 
					  << r.response << "\" to " << *r.destination << std::endl;
			if (iResult == SOCKET_ERROR) {
				std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
				closesocket(*r.destination);
				WSACleanup();
			}
		}

		// Something happenned
		if (select(NULL, &readfds, &writefds, &exceptfds, 0) > 0) {
			// Is it listen_socket?
			if (FD_ISSET(listen_socket, &readfds)) {
				SOCKET client_socket;
				client_socket = accept(listen_socket, NULL, NULL);
				if (client_socket == INVALID_SOCKET) {
					std::cerr << "accept() failed with error: " << WSAGetLastError() << std::endl;
					WSACleanup();
					return;
				}
				std::cout << timestamp() << " new connection initiated (" << client_socket << ")" << std::endl;
				client_sockets.push_back(Connection(client_socket));
			}

			// Is it one of the client sockets?
			ConnectionList::iterator it = client_sockets.begin();
			while (it != client_sockets.end()) {
				bool ok = true;
				if (FD_ISSET(it->socket, &readfds)) { 
					int iResult = recv(it->socket, it->buffer + it->chars_in_buffer, BUFLEN - it->chars_in_buffer, 0);

					std::string command_name = parse_buffer(it->buffer + it->chars_in_buffer);
					it->resetBuffer();
					qr.push(Request(command_name, &it->socket));

					if (iResult == 0) {
						std::cout << timestamp() << " socket " << it->socket << " was closed by client" << std::endl;
						ok = false;
					}
					if (iResult == -1) {
						std::cout << timestamp() << " something happenned to the socket "
							      << it->socket << std::endl;
						ok = iResult == WSAEWOULDBLOCK;
					}
					else {
						std::cout << timestamp() << " recieved \"" << command_name
								  << "\" from "  << it->socket << std::endl;
						it->chars_in_buffer += iResult;
					}

					FD_CLR(it->socket, &readfds);
				}

				if (FD_ISSET(it->socket, &writefds)) {  }
				
				if (!ok) {
					std::cout << timestamp() << " connection closed (" << it->socket << ")" << std::endl;
					closesocket(it->socket);
					client_sockets.erase(it);
					it = client_sockets.begin();
				}
				else {
					++it;
				}
			}
		}
	}
}

int main() {
	RequestQueue qr;
	ResponseQueue qp;
	std::thread worker(worker_thread, std::ref(qr), std::ref(qp));

	tinyxml2::XMLDocument doc;
	doc.LoadFile("commands.xml");

	std::map<std::string, std::string> commands;

    for (const XMLNode *node = doc.FirstChildElement("commands")->FirstChild(); 
		node; 
		node = node->NextSiblingElement()) {
		std::string command(node->FirstChildElement("name")->GetText());
		std::string response(node->FirstChildElement("response")->GetText());
		commands[command] = response;
    }

	while (true) {
		{
			Request r = qr.pop();
			qp.push(Response(commands[r.command], r.initiator));
		}
	}

	return 0;
}