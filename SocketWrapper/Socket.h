#pragma once

#include <vector>
#include <functional>
#include <WinSock2.h>

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
	SOCKET *initiator;

	Request(std::string _command, SOCKET _initiator) :
		command(_command),
		initiator(_initiator) 
	{};
};

struct Response {
	std::string response;
	SOCKET *destination;

	Response(std::string _response, SOCKET* _destination) :
		response(_response),
		destination(_destination) 
	{};
};

enum SocketType { LISTENING, ESTABILISHED };


class ConnectionPool {
typedef std::vector<Connection> List;

public:
	bool is_ready();
	void reset();
	void accept();
	bool pending_conn_present();
	void receive();
	void receive(std::function<void(char*, SOCKET)> dataHandler);

private:
	bool is_readable(SOCKET socket);

	List clients;
	Socket listening_socket;

	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;
};

class Socket {
public:
	Socket(SocketType t, int MAX_CONNECTIONS);
	Socket(SOCKET _s) : s(_s) {};

	bool is_readable();
	bool is_writeable();
	bool is_error();

	Socket accept_connection();

	std::string ip();

private:
	int initialize_socket();
	void reset_fdsets();

	SOCKET s;
	PCSTR port;
};

