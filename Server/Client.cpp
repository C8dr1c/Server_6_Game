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
#include <iostream>
#include <thread>
#include "ThreadedSocket.h"
#include "Client.h"
#include "Output.h"
#include "Card.h"
#include <sstream>

#ifdef _WIN32
Client::Client(int id, SOCKET socket, const int MAXDATASIZE) : ThreadedSocket(socket, false, MAXDATASIZE), id(id)
{
	buffer = new char[MAXDATASIZE];

	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", id);
	std::string prefix = std::string(std::string(std::string("[CLIENT_") + numstr + std::string("] ")));
	output_prefix = (char*)malloc(strlen(prefix.c_str()) + 1);
	strcpy(output_prefix, prefix.c_str());
	playerName = "Joueur";
	playedLine = -1;
	playerPoints = 0;
}
#else
Client::Client(int id, int socket, const int MAXDATASIZE) : ThreadedSocket(socket, false, MAXDATASIZE), id(id)
{
	buffer = new char[MAXDATASIZE];

	char numstr[21]; // enough to hold all numbers up to 64-bits
	sprintf(numstr, "%d", id);
	std::string prefix = std::string(std::string(std::string("[CLIENT_") + numstr + std::string("] ")));
	output_prefix = (char*)malloc(strlen(prefix.c_str()) + 1);
	strcpy(output_prefix, prefix.c_str());
}
#endif

Client::~Client()
{
	ThreadedSocket::~ThreadedSocket();
	delete[] buffer;
	free(output_prefix);
	playedLine = -1;
}

bool Client::send_message(const char* buffer)
{
	if (socket_ == NULL || !is_alive)
		return false;

	if (send(socket_, buffer, strlen(buffer), 0) == -1) {
		Output::GetInstance()->print_error(output_prefix, "Error while sending message to client ");
		return false;
	}

	clientMessageState = ClientMessageState::WAITING_MESSAGE_OK;
	return true;
}
bool Client::isMessageReady() const
{
	return clientMessageState == ClientMessageState::READY;
}
int Client::recv_message()
{
	if (socket_ == NULL || !is_alive)
		return -1;

	int length;
	if ((length = recv(socket_, buffer, MAXDATASIZE, 0)) == -1) {
		Output::GetInstance()->print_error(output_prefix, "Error while receiving message from client ");
		return length;
	}

	// Suppression des retours chariots (\n et \r)
	while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
		length--;
	// Ajout de backslash zero a la fin pour en faire une chaine de caracteres
	if (length >= 0 && length < MAXDATASIZE)
		buffer[length] = '\0';

	return length;
}

void Client::end_thread()
{
	if (!is_alive)
		return;

	// Sending close connection to client
	send_message("CONNECTION_CLOSED");
	clientMessageState = ClientMessageState::DISCONNECT;
	ThreadedSocket::end_thread();
}

int Client::getID() const 
{
	return id;
}

void Client::SetPlayedCard(Card* card) {
	playedCard = card;
}

Card* Client::getPlayedCard() const {
	return playedCard;
}

void Client::SetPlayedLine(int i) {
	playedLine = i;
}

int Client::getPlayedLine() const {
	return playedLine;
}

vector<string> split(const string& s, char delim) {
	vector<string> result;
	stringstream ss(s);
	string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}

	return result;
}

void Client::execute_thread()
{
	int length;
	time_t time_value;
	struct tm* time_info;

	Output::GetInstance()->print(output_prefix, "Thread client starts with id=", id, ".\n");

	// Boucle infinie pour le client
	while (1) {

		if (socket_ == NULL || !is_alive)
			return;

		// On attend un message du client
		if ((length = recv_message()) == -1) {
			break;
		}

		if (socket_ == NULL || !is_alive)
			return;

		// Affichage du message
		//Output::GetInstance()->print(output_prefix, "Message received : ", buffer, "\n");

		if (strcmp(buffer, "DISCONNECT") == 0) {
			break;
		}
		else {
			// On recupere l'heure et la date
			time(&time_value);
			time_info = localtime(&time_value);

			string msg(buffer);

			vector<string> splitedMsg = split(msg, ':');
			string command(splitedMsg.at(0));
			
			// Traitement du message reçu
			if (strcmp(buffer, "OK") == 0) {
				clientMessageState = ClientMessageState::READY;
			}
			else if (strcmp(command.c_str(), "PLAY") == 0) {
				Card* c;
				for (Card* card : playerCards) {
					if (card->getValue() == stoi(splitedMsg.at(1))) {
						c = card;
					}
				}

				if (find(playerCards.begin(), playerCards.end(), c) != playerCards.end()) {
					playedCard = c;
				}
				else {
					//sprintf(buffer, "This player does not have this card %s", buffer);
					send_message("NO:This player does not have this card %s");
				}	
			}	
			else if (strcmp(command.c_str(), "LINE") == 0) {
				int pLine = stoi(splitedMsg.at(1));

				if (pLine <= 3 && pLine >= 0) {
					playedLine = pLine;
				}
				else {
					//sprintf(buffer, "The line must be between 0 and 3", buffer);
					send_message("NO:The line must be between 0 and 3");
				}
			}
			//Join
			else if (strcmp(command.c_str(), "JOIN") == 0) {
				playerName = splitedMsg.at(1);
			}
			//Not valid command
			else {
				//sprintf(buffer, "NO");
				send_message("NO:Unknown command!");
			}

			if (socket == NULL || !is_alive)
				return;
		}
	}

	end_thread();
}
