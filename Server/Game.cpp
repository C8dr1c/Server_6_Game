
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
#include "ThreadedSocket.h"
#include "EndPoint.h"
#include "Game.h"
#include "Client.h"
#include "Output.h"
#include "Card.h"

Game::Game(EndPoint* connection, const int MAXDATASIZE, bool init_winsocks) : ThreadedSocket(NULL, init_winsocks, MAXDATASIZE)
{
    output_prefix = (char*)malloc(strlen("[GAME] ") + 1);
    strcpy(output_prefix, "[GAME] ");
    connection_ = connection;
}

Game::~Game()
{
    ThreadedSocket::~ThreadedSocket();
    free(output_prefix);
}

void Game::end_thread()
{
    if (!is_alive)
        return;

    ThreadedSocket::end_thread();
}

int currentTimeMillis() {
    FILETIME f;
    GetSystemTimeAsFileTime(&f);
    (long long)f.dwHighDateTime;
    __int64 nano = ((__int64)f.dwHighDateTime << 32LL) + (__int64)f.dwLowDateTime;
    return (nano - 116444736000000000LL) / 10000;
}

void Game::execute_thread()
{
    Output::GetInstance()->print(output_prefix, "Thread game starts.\n");

    int gameTime = currentTimeMillis();
    int gameSeconds = 0;

    int gameTurn = 10;
    int currentTurn = 0;

    vector<Client*> clients = connection_->getClients();

    while (1)
    {
        if (!is_alive)
            return;

        int currentTime = currentTimeMillis();
        int currentSeconds = abs(gameTime - currentTime) / 1000;

        switch (gameState) {
            case GameState::WAITING_PLAYERS:
            {
                if(clients.size() >= 2 && currentSeconds > 30) {
                    Output::GetInstance()->print(output_prefix, "Game starting !\n");
                    gameState = GameState::IN_GAME;
                    break;
                }
                break;
            }
            case GameState::START_GAME:
            {

                //On crée les cartes
                for (int i = 1; i <= 104; i++) {
                    game_cards.push_back(make_shared<Card>(Card(i)));
                    shuffle(begin(game_cards), end(game_cards), rand());
                }

                //On Distribue les cartes aux joueurs
                for (Client* client : clients) {
                    string msg("CARDS:");

                    for (int i = 0; i <= 10; i++) {
                        Card card = *game_cards.back().get();
                        //On enleve la dernière carte
                        game_cards.pop_back();

                        //On la rajoute au client
                        client->playerCards.push_back(card);

                        msg += card.getValue();
                        msg += ",";
                    }
                    //On envoi les cartes aux clients
                    client->send_message(msg.c_str());
                }
                /*
                //On distribue 4 cartes pour les 4 piles
                int randomIndex = rand() % game_cards.size();
                game_cards_line_1.push_back(game_cards.at(randomIndex));
                game_cards.erase(&game_cards[randomIndex]);

                randomIndex = rand() % game_cards.size();
                game_cards_line_2.push_back(game_cards.at(randomIndex));
                game_cards.erase(&game_cards[randomIndex]);

                randomIndex = rand() % game_cards.size();
                game_cards_line_3.push_back(game_cards.at(randomIndex));
                game_cards.erase(&game_cards[randomIndex]);

                randomIndex = rand() % game_cards.size();
                game_cards_line_4.push_back(game_cards.at(randomIndex));
                game_cards.erase(&game_cards[randomIndex]);*/
            }
            case GameState::IN_GAME:
            {
                //Boucle 1 fois par secondes 
                if (gameSeconds != currentSeconds) {
                    gameSeconds++;

                    if (gameTurn <= 0) {
                        gameState = END_GAME;
                        break;
                    }
                    else {
                        if (gameTurn != currentTurn) {
                            switch (turnState) {

                            //On Attend les cartes jouées
                            case TurnState::WAITING_PLAYERS_CARDS:
                            {
                                bool canContinue = true;

                                for (Client* client : clients) {

                                    if (client->playedCards.find(gameTurn) != 0) {
                                        canContinue = false;
                                    }
                                }


                                if (canContinue) {
                                    turnState = TurnState::SEND_BOARD_CARDS;
                                    break;
                                }
                                break;
                            }

                            //On renvoi les cartes du plateau suite aux cartes jouées par les joueurs
                            case TurnState::SEND_BOARD_CARDS:
                            {




                                gameTurn++;
                                turnState = TurnState::WAITING_PLAYERS_CARDS;
                                break;
                            }
                            }
                        }
                    }
                }
                break;
            }
            case GameState::END_GAME:
            {
                Output::GetInstance()->print(output_prefix, "Game finished !\n");
                Output::GetInstance()->print("\n");
                Output::GetInstance()->print("\n");
                Output::GetInstance()->print("*********************************************************\n");
                Output::GetInstance()->print("*                         SCORE                         *\n");
                Output::GetInstance()->print("*********************************************************\n");
                Output::GetInstance()->print("* 1.                                                    *\n");
                Output::GetInstance()->print("* 2.                                                    *\n");
                Output::GetInstance()->print("* 3.                                                    *\n");
                Output::GetInstance()->print("* 4.                                                    *\n");
                Output::GetInstance()->print("* 5.                                                    *\n");
                Output::GetInstance()->print("* 6.                                                    *\n");
                Output::GetInstance()->print("* 7.                                                    *\n");
                Output::GetInstance()->print("* 8.                                                    *\n");
                Output::GetInstance()->print("* 9.                                                    *\n");
                Output::GetInstance()->print("* 10.                                                   *\n");
                Output::GetInstance()->print("*********************************************************\n");

            }
        }
    }
}
