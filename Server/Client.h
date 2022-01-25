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
#include "Card.h"
#include <map>

#ifndef CLIENT_H
#define CLIENT_H

using namespace std;

class Client : public ThreadedSocket
{
protected:
	int id;
	char* buffer;

	Card* playedCard;
	int playedLine;

	void execute_thread();

	int recv_message();

	enum ClientMessageState { READY, WAITING_MESSAGE_OK };
	ClientMessageState clientMessageState;

public:
#ifdef _WIN32
	Client(int, SOCKET, const int MAXDATASIZE);
#else
	Client(int, int, const int MAXDATASIZE);
#endif
	~Client();
	void end_thread();

	bool send_message(const char*);

	vector<Card*> playerCards;
	int playerPoints;
	string playerName;

	int getID() const;

	void SetPlayedCard(Card* c);
	Card* getPlayedCard() const;

	void SetPlayedLine(int i);
	int getPlayedLine() const;

	bool isMessageReady() const;
};

#endif
