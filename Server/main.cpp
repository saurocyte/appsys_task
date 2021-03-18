#define _WINSOCK_DEPRECATED_NO_WARNINGS

#pragma comment(lib, "ws2_32.lib")

#include "Server.h"

int main() {
	Server s;
	s.run();

	return 0;
}