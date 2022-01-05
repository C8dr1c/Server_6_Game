
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <thread>
#include <vector>
#include "ThreadedSocket.h"
#include "Client.h"

#ifndef ENDPOINT_H
#define ENDPOINT_H

class EndPoint : public ThreadedSocket
{
	const int BACKLOG;
	int connection_port;
	std::vector<Client*> clients;

	bool open_socket();
#ifdef _WIN32
	SOCKET accept_connection();
#else
	int accept_connection();
#endif
	void execute_thread();

public:
	EndPoint(int, const int, const int, bool);
	~EndPoint();
	void end_thread();
	std::vector<Client*> getClients() const;

};

#endif
