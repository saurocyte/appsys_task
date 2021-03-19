#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS


#include "Client.h"

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
