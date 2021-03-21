#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <vector>
#include <functional>
#include <WinSock2.h>
#include <string>
#include <array>

typedef SOCKET;

class Connection {
private:
	static const int BUFLEN = 512;

public:
	typedef std::array<char, BUFLEN> buffer_t;

	SOCKET socket;

	buffer_t buffer;
	
	int recieve();
	void reset_buffer();

	Connection(SOCKET _socket) : 
		socket(_socket),
		buffer{ 0 } {};
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

class ServerSocket {
public:
	ServerSocket(int MAX_CONNECTIONS, PCSTR port);

	std::string ip();

	SOCKET s;
private:
	int initialize();

	PCSTR port;
};

class ClientSocket {
public:
	ClientSocket(PCSTR addr, PCSTR port);

	SOCKET s;
};

class ConnectionPool {
typedef std::vector<Connection> List;

public:
	ConnectionPool(unsigned int MAX_CONNECTIONS);

	bool is_ready();
	void reset();
	void accept();
	bool pending_conn_present();
	void receive(std::function<void(Connection::buffer_t, SOCKET)> dataHandler);
	std::string ip();

private:
	const PCSTR PORT = "27015";

	bool is_readable(SOCKET socket);

	List clients;
	ServerSocket listening_socket;

	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
};
