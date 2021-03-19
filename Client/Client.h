#pragma once

#include <WinSock2.h>
#include <vector>
#include <string>
#include "SocketWrapper/MessageParser.h"
#include "SocketWrapper/Socket.h"

typedef char Int8;
typedef short int Int16;
typedef int Int32;

typedef std::vector<Command> CommandList;

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
