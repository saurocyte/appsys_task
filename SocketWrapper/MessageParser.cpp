#include "pch.h"
#include "MessageParser.h"

Command::Command(std::string _name) : name(_name) {};

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


Int8* MessageParser::encode(Command cmd) {
	Int16 name_size = cmd.name.length();
	Int8 *data = new char[2 + name_size];
	data[0] = (name_size >> 8) & 0xff;
	data[1] = name_size & 0xff;
	for (auto i = 0; i < name_size; ++i) {
		data[2 + i] = cmd.name[i];
	}
	return data;
}
