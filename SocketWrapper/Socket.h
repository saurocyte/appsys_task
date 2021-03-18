#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <vector>
#include <functional>
#include <WinSock2.h>
#include <string>

typedef SOCKET;

struct Connection {
	static const int BUFLEN = 512;

	SOCKET socket;
	char buffer[BUFLEN];
	u_int chars_in_buffer;

	int recieve() {
		int iResult = recv(socket, buffer + chars_in_buffer, BUFLEN - chars_in_buffer, 0);
		return iResult;
	}

	void resetBuffer() {
		buffer[0] = '\0';
		chars_in_buffer = 0;
	}

	Connection(SOCKET _socket) : 
		socket(_socket),
		buffer{ 0 },
		chars_in_buffer(0) {};
};

struct Request {
	std::string command;
	SOCKET initiator;

	Request(std::string _command, SOCKET _initiator) :
		command(_command),
		initiator(_initiator) 
	{};
};

struct Response {
	std::string response;
	SOCKET destination;

	Response(std::string _response, SOCKET _destination) :
		response(_response),
		destination(_destination) 
	{};
};

class ListeningSocket {
public:
	ListeningSocket(int MAX_CONNECTIONS, PCSTR port);

	std::string ip();

	SOCKET s;
private:
	int initialize();

	PCSTR port;
};


class ConnectionPool {
typedef std::vector<Connection> List;

public:
	ConnectionPool(unsigned int MAX_CONNECTIONS);

	bool is_ready();
	void reset();
	void accept();
	bool pending_conn_present();
	void receive(std::function<void(char*, SOCKET)> dataHandler);
	std::string ip();

private:
	const PCSTR PORT = "27015";

	bool is_readable(SOCKET socket);

	List clients;
	ListeningSocket listening_socket;

	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
};
