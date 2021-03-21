#pragma once

#include <string>
#include <stdexcept>

class SocketError : public std::runtime_error {
public:
	SocketError() : runtime_error("Unable to connect to server") {};
	SocketError(std::string  msg) : runtime_error(msg.c_str()) {};
};
