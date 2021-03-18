#pragma once

typedef SOCKET;

struct Connection {
	SOCKET socket;
	char buffer[BUFLEN];
	u_int chars_in_buffer;

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

	Request(std::string _command, SOCKET* _initiator) :
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

class Socket {
public:
	Socket(SocketType t, int MAX_CONNECTIONS);
	Socket(SOCKET _s) : s(_s) {};

	bool is_ready();
	bool is_readable();
	bool is_writeable();
	bool is_error();

	Socket accept_connection();

	std::string get_ip();

private:

	int initialize_socket();
	void reset_fdsets();

	fd_set readfds;
	fd_set writefds;
	fd_set exceptfds;

	SOCKET s;
	PCSTR port;
};

