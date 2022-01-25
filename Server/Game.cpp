
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
#include <algorithm>
#include <string>
#include <chrono>

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
    int nano = ((int)f.dwHighDateTime << 32L) + (int)f.dwLowDateTime;
    return (nano - 116444736000000000LL) / 10000;
}

void Game::execute_thread()
{
    Output::GetInstance()->print(output_prefix, "Thread game starts.\n");

    int gameTurn = 10;

    bool gameStarting = false;
    int gameStartingSeconds = 2;

    vector<Client*> clientsMessageSend;
    vector<Client*> playedClients;

    gameState = GameState::WAITING_PLAYERS;

    while (1)
    {

        if (!is_alive)
            return;

        int currentTime = currentTimeMillis();

        //On récup les clients
        vector<Client*> clients = connection_->getClients();

        switch (gameState) {
            case GameState::WAITING_PLAYERS:
            {
                if (currentTime % 1000 == 0) {
                    if (gameStarting) {
                        //Démarrage dans 30 Secondes!
                        if (gameStartingSeconds <= 0) {
                            Output::GetInstance()->print(output_prefix, "Initialise Game !\n");
                            gameState = GameState::INIT;

                            //On bloque les prochains clients qui voudront se connecter
                            connection_->gameStart = true;
                            break;
                        }
                        else if (gameStartingSeconds <= 5) {
                            Output::GetInstance()->print(output_prefix, "Game start in ", gameStartingSeconds, " seconds !\n");
                        }
                        gameStartingSeconds--;

                        //Si le nombre de joueur et supérieur à 2 on lance le compte à rebours 
                    }
                    else if (clients.size() >= 1) {
                        Output::GetInstance()->print(output_prefix, "Game start in ", gameStartingSeconds, " seconds !\n");
                        gameStarting = true;
                    }
                }
                break;
            }
            case GameState::INIT:
            {
                //On crée les cartes
                for (int i = 1; i <= 104; i++) {
                    game_cards.push_back((new Card(i)));
                }
                //On mélange les cartes
                default_random_engine rand(currentTime);
                shuffle(game_cards.begin(), game_cards.end(), rand);

                //On distribue les cartes aux joueurs
                for (Client* client : clients) {

                    string msg("CARDS:");

                    for (int i = 0; i < gameTurn; i++) {
                        //On récupère la dernière carte de la pioche
                        Card* card = game_cards.back();
                        //On enleve la dernière carte de la pioche
                        game_cards.pop_back();

                        //On la rajoute au client
                        client->playerCards.push_back(card);
                    }
                }

                //On distribue 4 cartes pour les 4 piles
                for (int i = 0; i < 4; i++) {
                    vector<Card*> line_cards;
                    line_cards.push_back(game_cards.back());
                    game_board_cards.push_back(line_cards);
                    game_cards.pop_back();
                }

                gameState = GameState::NEXT_TURN;
                break;
            }
            case GameState::NEXT_TURN:
            {
                if (gameTurn <= 0) {
                    gameState = END_GAME;
                    break;
                }
                Output::GetInstance()->print(output_prefix, "Round ", gameTurn, " !\n");
                gameTurn--;
                gameState = GameState::SEND_CARDS;
                break;
            }
            case GameState::SEND_CARDS:
            {
                //Si les cartes ont étaient envoyé à tout le monde
                if (clientsMessageSend.size() == clients.size()) {
                    clientsMessageSend.clear();
                    //Output::GetInstance()->print(output_prefix, "Dealt cards !\n");
                    gameState = GameState::SEND_BOARD;
                    break;
                }

                for (Client* client : clients) {

                    if (find(clientsMessageSend.begin(), clientsMessageSend.end(), client) != clientsMessageSend.end()) {
                        continue;
                    }

                    string msg("CARDS:");

                    for (Card* card : client->playerCards) {
                        msg += to_string(card->getValue());
                        msg += ",";
                    }

                    if (client->isMessageReady()) {
                        //On envoi les cartes au joueur
                        Output::GetInstance()->print(output_prefix, "Sending cards to player ID:", client->getID(), " (", client->playerName, ") ", " ", msg, " !\n");
                        client->send_message(msg.c_str());
                        clientsMessageSend.push_back(client);
                    }
                }
                break;
            }
            case GameState::SEND_BOARD:
            {
                //Si le board à était envoyé à tout le monde
                if (clientsMessageSend.size() == clients.size()) {
                    clientsMessageSend.clear();
                    //Output::GetInstance()->print(output_prefix, "Board send !\n");
                    gameState = GameState::WAITING_PLAYER_CARDS;
                    break;
                }

                //On serialize les lignes pour les envoyées aux clients
                string board_cards;
                for (int i = 0; i < 4; i++) {
                    for (Card* card : game_board_cards.at(i))
                    {
                        board_cards += to_string(card->getValue());
                        board_cards += ",";
                    }
                    board_cards += ";";
                }

                string board("BOARD:" + board_cards);

                for (Client* client : clients) {

                    if (find(clientsMessageSend.begin(), clientsMessageSend.end(), client) != clientsMessageSend.end()) {
                        continue;
                    }

                    if (client->isMessageReady()) {
                        //On envoi le board aux clients
                        Output::GetInstance()->print(output_prefix, "Sending board to player ID:", client->getID(), " (", client->playerName, ") ", " ", board, " !\n");
                        client->send_message(board.c_str());
                        clientsMessageSend.push_back(client);
                    }
                }
                break;
            }
            case GameState::WAITING_PLAYER_CARDS:
            {
                bool canContinue = true;

                for (Client* client : clients) {
                    if (client->getPlayedCard() != NULL) {
                        if (client->getPlayedCard()->getValue() > 0) {
                            continue;
                        }
                    }
                    canContinue = false;
                }

                if (canContinue) {
                    gameState = GameState::CHECK_LINES;
                }
                break;
            }
            case GameState::CHECK_LINES:
            {
                //Si tout les joueurs on joué
                if (playedClients.size() == clients.size()) {
                    for (Client* client : playedClients) {
                        client->SetPlayedCard(NULL);
                    }
                    playedClients.clear();
                    gameState = GameState::NEXT_TURN;
                    break;
                }

                Client* currentLowestPlayedCardClient = clients.at(0);
                Card* currentLowestPlayedCard = currentLowestPlayedCardClient->getPlayedCard();

                for (Client* client : clients) {

                    if (find(playedClients.begin(), playedClients.end(), client) != playedClients.end()) {
                        if (currentLowestPlayedCardClient == client) currentLowestPlayedCardClient = NULL;
                        continue;
                    }

                    Card* playedCard = client->getPlayedCard();

                    Output::GetInstance()->print("Player ID: ", client->getID(), " (", client->playerName, ") ", " played card ",
                        playedCard->getValue(),
                        " - (", playedCard->getPoints(), " pts)", "!\n");

                    if (currentLowestPlayedCard->getValue() > playedCard->getValue() || currentLowestPlayedCardClient == NULL) {
                        currentLowestPlayedCardClient = client;
                        currentLowestPlayedCard = playedCard;
                    }
                }

                //On retire la carte au joueur ayant la plus petite carte
                currentLowestPlayedCardClient->playerCards.erase(
                    remove(currentLowestPlayedCardClient->playerCards.begin(),
                        currentLowestPlayedCardClient->playerCards.end(),
                        currentLowestPlayedCard),
                    currentLowestPlayedCardClient->playerCards.end());

                int bestLine = 0;

                //On recherche la meilleur ligne
                for (int i = 0; i < 4; i++) {
                    if (currentLowestPlayedCard->getValue() < game_board_cards.at(i).back()->getValue()) continue;
                    if (currentLowestPlayedCard->getValue() < game_board_cards.at(bestLine).back()->getValue()) {
                        bestLine = i;
                        continue;
                    }

                    int currentPoints = currentLowestPlayedCard->getValue() - game_board_cards.at(i).back()->getValue();
                    int bestPoints = currentLowestPlayedCard->getValue() - game_board_cards.at(bestLine).back()->getValue();

                    if (currentPoints < bestPoints) bestLine = i;
                }


                playedClients.push_back(currentLowestPlayedCardClient);

                //Si pas de ligne correcte on demande au joueur de choisir ça ligne
                if (currentLowestPlayedCard->getValue() < game_board_cards.at(bestLine).back()->getValue()) {
                    currentLowestPlayedCardClient->send_message("LINE");
                    gameState = GameState::TAKING_LINE;
                    break;
                }

                //Si la ligne à plus de 6 cartes
                if (game_board_cards.at(bestLine).size() + 1 >= 6) {
                    currentLowestPlayedCardClient->SetPlayedLine(bestLine);
                    gameState = GameState::TAKING_LINE;
                    break;
                }

                game_board_cards.at(bestLine).push_back(currentLowestPlayedCard);
                gameState = GameState::SEND_PLAY_BOARD;
                break;
            }
            case GameState::TAKING_LINE:
            {
                //si le joueur à choisi ça ligne
                if (playedClients.back()->getPlayedLine() != -1) {

                    //si les points on était envoyé au joueur
                    if (clientsMessageSend.size() == 1) {
                        clientsMessageSend.clear();
                        playedClients.back()->SetPlayedLine(-1);
                        gameState = GameState::SEND_PLAY_BOARD;
                        break;
                    }

                    if (playedClients.back()->isMessageReady()) {
                        //On envoi les points au client

                        //On calcule les points de la ligne
                        for (Card* card : game_board_cards.at(playedClients.back()->getPlayedLine()))
                        {
                            playedClients.back()->playerPoints += (int)card->getPoints();
                        }
                        game_board_cards.at(playedClients.back()->getPlayedLine()).clear();
                        game_board_cards.at(playedClients.back()->getPlayedLine()).push_back(playedClients.back()->getPlayedCard());

                        Output::GetInstance()->print(output_prefix, "Sending score to player ID:", playedClients.back()->getID(), playedClients.back()->getID(), " (", playedClients.back()->playerName, ") " , " SCORE = ", playedClients.back()->playerPoints, " !\n");
                        string msg("SCORE:" + to_string(playedClients.back()->playerPoints));
                        playedClients.back()->send_message(msg.c_str());
                        clientsMessageSend.push_back(playedClients.back());
                    }
                }
                break;
            }
            case GameState::SEND_PLAY_BOARD:
            {
                if (currentTime % 200 == 0) {
                    //Si le board à était envoyé à tout le monde
                    if (clientsMessageSend.size() == clients.size()) {
                        clientsMessageSend.clear();
                        //Output::GetInstance()->print(output_prefix, "Board send !\n");
                        gameState = GameState::CHECK_LINES;
                        break;
                    }

                    //On serialize les lignes pour les envoyées aux clients
                    string board_cards;
                    for (int i = 0; i < 4; i++) {
                        for (Card* card : game_board_cards.at(i))
                        {
                            board_cards += to_string(card->getValue());
                            board_cards += ",";
                        }
                        board_cards += ";";
                    }

                    string board("BOARD:" + board_cards);

                    for (Client* client : clients) {

                        if (find(clientsMessageSend.begin(), clientsMessageSend.end(), client) != clientsMessageSend.end()) {
                            continue;
                        }

                        if (client->isMessageReady()) {
                            //On envoi le board aux clients
                            Output::GetInstance()->print(output_prefix, "Sending board to player ID:", client->getID(), " (", client->playerName, ") ", board, " !\n");
                            client->send_message(board.c_str());
                            clientsMessageSend.push_back(client);
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

                for (Client* client : clients) {
                    Output::GetInstance()->print("*               ", client->playerName, ". ", client->playerPoints, " Pts!", "\n");
                }

                Output::GetInstance()->print("*********************************************************\n");
                gameState = GameState::END;
                break;
            }
            case GameState::END:
            {
                if (clientsMessageSend.size() == clients.size()) {
                    break;
                }

                for (Client* client : clients) {

                    if (find(clientsMessageSend.begin(), clientsMessageSend.end(), client) != clientsMessageSend.end()) {
                        continue;
                    }

                    string end("END:");
                    if (client->isMessageReady()) {
                        //On envoi le board aux clients
                        for (Client* client : clients) {
                            end += client->playerName;
                            end += ",";
                            end += client->playerPoints;
                            end += ";";
                        }
                    }

                    client->send_message(end.c_str());
                    clientsMessageSend.push_back(client);
                }
                break;
            }
        }
    }
}
