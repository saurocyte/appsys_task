#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment (lib, "ws2_32.lib")

#include <stdexcept>

#include "Client.h"


int main(int argc, char **argv) 
{
    if (argc != 2) {
        throw std::runtime_error("usage: " + std::string(argv[0]) + " server-name");
    }
    
    Client client(argv[1], "27015", "commands.xml");

    client.run();
    
    return 0;
}
