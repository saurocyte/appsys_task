﻿#define _WINSOCK_DEPRECATED_NO_WARNINGS
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
#include <string>

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>

#include "../tinyxml2.h"

#include "SocketWrapper/Socket.h"
#include "SocketWrapper/MessageParser.h"
#include "timestamp/timestamp.h"

#pragma comment (lib, "ws2_32.lib")

typedef char Int8;
typedef short int Int16;
typedef int Int32;

typedef std::vector<Command> CommandList;

using namespace tinyxml2;

class Client {
public:
    Client(PCSTR addr, PCSTR port, const std::string _commands_path);
    void run();
private:
    void parse_commands();

    std::string commands_path;
    CommandList commands;

    ClientSocket conn;
    char recvbuf[512];
    int recvbuflen = 512;
};

Client::Client(PCSTR addr, PCSTR port, const std::string _commands_path)
: conn(addr, port), commands_path(_commands_path) {
	tinyxml2::XMLDocument doc;
    doc.LoadFile("commands.xml");

    for (const XMLNode *node = doc.FirstChildElement("commands")->FirstChild(); 
        node; 
        node = node->NextSiblingElement()) {
        std::string name = std::string(node->ToElement()->GetText());
        
        commands.push_back(Command(name));
    }
}

void Client::run() {
    auto it = commands.begin();

    while (true) {
        Int8* data = MessageParser::encode(*it);

        std::cout << Timestamp::timestamp() <<  " sending \"" << it->name << "\"" << std::endl;
        int iResult = send(conn.s, data, 2 + it->name.length(), 0);
		if (iResult == SOCKET_ERROR) {
			std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
			closesocket(conn.s);
			WSACleanup();
			return;
		}

        char buffer[512];
        iResult = recv(conn.s, buffer, sizeof(buffer), 0);
        std::cout << Timestamp::timestamp() << " got " << buffer << std::endl;

        delete data;

        ++it;
        if (it == commands.end()) {
            it = commands.begin();
        }

        auto end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // shutdown the connection since no more data will be sent
    int iResult = shutdown(conn.s, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(conn.s);
        WSACleanup();
        return;
    }

    // cleanup
    closesocket(conn.s);
    WSACleanup();
}

int main(int argc, char **argv) 
{
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }
    
    Client client(argv[1], "27015", "commands.xml");

    client.run();
    
    return 0;
}
