#pragma once
#include <string>

typedef char Int8;
typedef short int Int16;
typedef int Int32;

struct Command {
    std::string name;

    Command(std::string _name);
};


class MessageParser {
public:
	static std::string decode(char* buffer);
	static Int8 *encode(Command cmd);
};

