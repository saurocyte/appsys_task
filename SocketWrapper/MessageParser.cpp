#include "MessageParser.h"
#include <vector>

Command::Command(std::string _name) : name(_name) {};

std::string MessageParser::decode(Connection::buffer_t buffer) {
	Int16 name_size = buffer[0];
	name_size = name_size << 8;
	name_size |= buffer[1];

	std::string command_name = "";
	for (int i = 0; i < name_size; ++i) {
		command_name += buffer[2 + i];
	}

	return command_name;
}


std::vector<Int8> MessageParser::encode(Command cmd) {
	Int16 name_size = cmd.name.length();
	std::vector<Int8> data;
	data.push_back((name_size >> 8) & 0xff);
	data.push_back(name_size & 0xff);
	for (auto i = 0; i < name_size; ++i) {
		data.push_back(cmd.name[i]);
	}
	return data;
}
