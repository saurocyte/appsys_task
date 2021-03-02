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

#include "../tinyxml2.h"

#pragma comment (lib, "ws2_32.lib")

#define BUFLEN 512
#define PORT "27015"

typedef char Int8;
typedef short int Int16;
typedef int Int32;

struct Command {
    const char *name;

    Command(const char* _name) : name(_name) {};
};

typedef std::vector<Command> CommandList;

auto timestamp() {
	time_t now = time(nullptr) ;
	return std::put_time(localtime(&now), "%T");
}

using namespace tinyxml2;

int main(int argc, char **argv) 
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    char recvbuf[BUFLEN];
    int iResult;
    int recvbuflen = BUFLEN;

    // Validate the parameters
    if (argc != 2) {
        printf("usage: %s server-name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for(ptr=result; ptr != NULL; ptr=ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    CommandList commands;
	tinyxml2::XMLDocument doc;

	doc.LoadFile("commands.xml");

    for (const XMLNode *node = doc.FirstChildElement("commands")->FirstChild(); 
        node; 
        node = node->NextSiblingElement()) {
        const char* name = node->ToElement()->GetText();
        
        commands.push_back(Command(name));
    }

    auto it = commands.begin();

    while (true) {
        Int16 name_size = strlen(it->name);
        Int8 *data = new char[2 + name_size];
        data[0] = (name_size >> 8) & 0xff;
        data[1] = name_size & 0xff;
        for (auto i = 0; i < name_size; ++i) {
            data[2 + i] = it->name[i];
        }

        std::cout << timestamp() <<  " sending \"" << it->name << "\"" << std::endl;
        iResult = send(ConnectSocket, data, 2 + name_size, 0);
		if (iResult == SOCKET_ERROR) {
			std::cerr << "send failed with error: " << WSAGetLastError() << std::endl;
			closesocket(ConnectSocket);
			WSACleanup();
			return 1;
		}

        char buffer[512];
        iResult = recv(ConnectSocket, buffer, sizeof(buffer), 0);
        std::cout << timestamp() << " got " << buffer << std::endl;

        delete data;

        ++it;
        if (it == commands.end()) {
            it = commands.begin();
        }

        auto end = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {
        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if ( iResult > 0 )
            printf("Bytes received: %d\n", iResult);
        else if ( iResult == 0 )
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while( iResult > 0 );

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    while (true) {};
    
    return 0;
}
