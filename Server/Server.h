#pragma once

#include "SafeQueue.h"
#include "SocketWrapper/Socket.h"

struct Request;
struct Response;

typedef SafeQueue<Request> RequestQueue;
typedef SafeQueue<Response> ResponseQueue;
typedef char Int8;
typedef int Int32;

class Server {
public:
	Server() = default;

	void run();

private:
	RequestQueue qr;
	ResponseQueue qp;

	void respond();

	static void worker_thread(RequestQueue &qr, ResponseQueue &qp);
};
