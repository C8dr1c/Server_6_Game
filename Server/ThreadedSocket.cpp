
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
#include "ThreadedSocket.h"
#include "Output.h"

#ifdef _WIN32
ThreadedSocket::ThreadedSocket(SOCKET socket, bool init_winsocks, const int MAXDATASIZE) :  socket_(socket), init_winsocks(init_winsocks), MAXDATASIZE(MAXDATASIZE), is_alive(true), output_prefix(NULL)
{
}
#else
ThreadedSocket::ThreadedSocket(int socket, bool init_winsocks, const int MAXDATASIZE) : socket_(socket), init_winsocks(init_winsocks), MAXDATASIZE(MAXDATASIZE), is_alive(true), output_prefix(NULL)
{
}
#endif

ThreadedSocket::~ThreadedSocket()
{
	end_thread();
}

bool ThreadedSocket::close_socket()
{
	if (socket == NULL || !is_alive)
		return true;

	int result;
	Output::GetInstance()->print(output_prefix, "Closing client socket...\n");

#ifdef _WIN32
	result = closesocket(socket_);
#else
	result = close(socket_);
#endif

	if (result == -1) {
		Output::GetInstance()->print_error(output_prefix, "Error while closing socket ");
		return false;
	}

#ifdef _WIN32
	if (init_winsocks && WSACleanup() == -1) {
		Output::GetInstance()->print_error(output_prefix, "Error while cleaning WinSocks ");
		return false;
	}
#endif

	Output::GetInstance()->print(output_prefix, "Socket closed successfully.\n");

	return true;
}

void ThreadedSocket::start_thread()
{
	join_thread();
	// Start client thread
	thread = std::thread(&ThreadedSocket::execute_thread, this);
}

void ThreadedSocket::end_thread()
{
	if (!is_alive)
		return;

	Output::GetInstance()->print(output_prefix, "Thread is ending...\n");

	// Calling ending_thread() -> child class implementation
	//ending_thread();

	is_alive = false;

	// End thread
	thread.detach();
	thread.~thread();

	// Close connection
	close_socket();

	Output::GetInstance()->print(output_prefix, "Thread ends.\n");
}

void ThreadedSocket::join_thread()
{
	if (thread.joinable()) {
		thread.join();
	}
}
