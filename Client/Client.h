#pragma once

#include <WinSock2.h>
#include <string>

#include "SocketWrapper/Socket.h"
#include "SocketWrapper/MessageParser.h"

typedef char Int8;
typedef short int Int16;
typedef int Int32;

typedef std::vector<Command> CommandList;

class Client {
public:
    Client(std::string addr, std::string port, const std::string _commands_path);
    void run();
private:
    std::string commands_path;
    CommandList commands;

    ClientSocket conn;
};


