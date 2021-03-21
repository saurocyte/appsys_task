#pragma once
#include <string>
#include <vector>
#include "Socket.h"

typedef char Int8;
typedef short int Int16;
typedef int Int32;

struct Command {
    std::string name;

    Command(std::string _name);
};


class MessageParser {
public:
	static std::string decode(Connection::buffer_t buffer);
	static Int8 *encode(Command cmd);
};

