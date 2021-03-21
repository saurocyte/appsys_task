#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <map>
#include <iterator>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "Server.h"
#include "../tinyxml2.h"

#include "SocketWrapper/Socket.h"
#include "SocketWrapper/MessageParser.h"
#include "timestamp/Timestamp.h"

using namespace tinyxml2;

void Server::worker_thread(RequestQueue &qr, ResponseQueue &qp) {
	ConnectionPool connections(10);

	// std::cout << timestamp() << " listening on " << listen_socket.ip() << ":" << PORT << std::endl;

	while (true) {
		connections.reset();

		// Respond to pending connections
		while (!qp.empty()) {
			Response r = qp.pop();
			int iResult = send(r.destination, r.response.c_str(), sizeof(r.response), 0);
			std::cout << Timestamp::timestamp() << " sent \"" 
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
			connections.receive([&](Connection::buffer_t buffer, SOCKET sender) {
				std::string command_name = MessageParser::decode(buffer);
				qr.push(Request(command_name, sender));
			});
		}
	}

}

void Server::run() {
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
}
