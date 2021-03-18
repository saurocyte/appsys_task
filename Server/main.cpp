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

#include "SocketWrapper/Socket.h"

#pragma comment(lib, "ws2_32.lib")


#define PORT "27015"
#define MAX_CONNECTIONS 10
#define BUFLEN 512

using namespace tinyxml2;
	
typedef SafeQueue<Request> RequestQueue;
typedef SafeQueue<Response> ResponseQueue;
typedef char Int8;
typedef short int Int16;
typedef int Int32;

class Server {

};

auto timestamp() {
	time_t now = time(nullptr) ;
	return std::put_time(localtime(&now), "%T");
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
	ConnectionPool connections(10);

	// std::cout << timestamp() << " listening on " << listen_socket.ip() << ":" << PORT << std::endl;

	while (true) {
		connections.reset();

		// Respond to pending connections
		while (!qp.empty()) {
			Response r = qp.pop();
			int iResult = send(r.destination, r.response.c_str(), sizeof(r.response), 0);
			std::cout << timestamp() << " sent \"" 
					  << r.response << "\" to " << r.destination << std::endl;
			if (iResult == SOCKET_ERROR) {
				std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
				closesocket(r.destination);
				WSACleanup();
			}
		}

		// Something happenned
		if (connections.is_ready()) {
			connections.accept();
			connections.receive([&](char* buffer, SOCKET sender) {
				std::string command_name = parse_buffer(buffer);
				qr.push(Request(command_name, sender));
			});
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