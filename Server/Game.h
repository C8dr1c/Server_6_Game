
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
#include "Card.h"

using namespace std;

#ifndef GAME_H
#define GAME_H

class Game : public ThreadedSocket
{

	EndPoint* connection_;

	void execute_thread();

	enum GameState { WAITING_PLAYERS, 
					 INIT, 
					 NEXT_TURN,
					 SEND_CARDS, 
					 SEND_BOARD, 
					 WAITING_PLAYER_CARDS,
					 SEND_PLAY_BOARD,
					 CHECK_LINES,
					 TAKING_LINE, 
					 SEND_END_CARDS,
					 END_GAME,
					 END};
	
	GameState gameState;

	vector<Card*> game_cards;
	vector<vector<Card*>> game_board_cards;

public:
	Game(EndPoint*, const int, bool);
	~Game();
	void end_thread();
};

#endif
