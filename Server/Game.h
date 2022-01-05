
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

	enum GameState { WAITING_PLAYERS, START_GAME, IN_GAME, END_GAME };
	enum TurnState { WAITING_PLAYERS_CARDS, SEND_BOARD_CARDS };

	GameState gameState;
	TurnState turnState;

	vector<shared_ptr<Card>> game_cards;

	vector<shared_ptr<Card>> game_cards_line_1;
	vector<shared_ptr<Card>> game_cards_line_2;
	vector<shared_ptr<Card>> game_cards_line_3;
	vector<shared_ptr<Card>> game_cards_line_4;

public:
	Game(EndPoint*, const int, bool);
	~Game();
	void end_thread();
};

#endif
