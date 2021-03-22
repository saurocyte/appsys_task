#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#pragma comment (lib, "ws2_32.lib")

#include <stdexcept>
#include "string"

#include "Client.h"

int main(int argc, char **argv) 
{
    std::string addr = "";
    if (argc == 2) {
        addr.assign(argv[1]);
    }
    else if (argc == 1) {
        addr = "localhost";
    }
    else {
        throw std::runtime_error("usage: " + std::string(argv[0]) + " server-name");
    }
    
    Client client(addr, "27015", "commands.xml");

    client.run();
    
    return 0;
}
