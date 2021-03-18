#include "pch.h"
#include "MessageParser.h"

typedef short int Int16;

std::string MessageParser::decode(char* buffer) {
	Int16 name_size = buffer[0];
	name_size = name_size << 8;
	name_size |= buffer[1];

	std::string command_name = "";
	for (int i = 0; i < name_size; ++i) {
		command_name += buffer[2 + i];
	}

	return command_name;
}

