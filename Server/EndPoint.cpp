
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
#include "Client.h"
#include "Output.h"

EndPoint::EndPoint(int connection_port, const int BACKLOG, const int MAXDATASIZE, bool init_winsocks) : ThreadedSocket(NULL, init_winsocks, MAXDATASIZE), connection_port(connection_port), BACKLOG(BACKLOG)
{
    output_prefix = (char*)malloc(strlen("[SERVER] ") + 1);
    strcpy(output_prefix, "[SERVER] ");
}

EndPoint::~EndPoint()
{
    ThreadedSocket::~ThreadedSocket();
    free(output_prefix);
}

#ifdef _WIN32
bool EndPoint::open_socket()
{
    SOCKADDR_IN address;

    // Initialisation l'utilisation des WinSocks par un processus
    if (init_winsocks) {
        WSADATA WSAData;
        if (WSAStartup(MAKEWORD(2, 0), &WSAData) == -1) {
            Output::GetInstance()->print_error(output_prefix, "Error while starting using WinSocks ");
            return false;
        }
    }

    // Ouverture de la socket de connexion
    if ((socket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while creating connection socket ");
        return false;
    }

    // Configuration de l'adresse de transport
    address.sin_addr.s_addr = INADDR_ANY;      // adresse, devrait être converti en reseau mais est egal à 0
    address.sin_family = AF_INET;              // type de la socket
    address.sin_port = htons(connection_port); // port, converti en reseau

    // Demarrage du point de connexion : on ajoute l'adresse de transport dans la socket
    if (bind(socket_, (SOCKADDR*)&address, sizeof(address)) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while binding connection socket ");
        return false;
    }

    // Attente sur le point de connexion
    if (listen(socket_, BACKLOG) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while listening connection socket ");
        return false;
    }

    return true;
}

SOCKET EndPoint::accept_connection()
{
    SOCKET client_socket;
    SOCKADDR_IN client_address;
    int sinsize = sizeof(client_address);

    // Acceptation de la connexion
    if ((client_socket = accept(socket_, (SOCKADDR*)&client_address, &sinsize)) == -1) {
        if (!is_alive)
            return NULL;
        Output::GetInstance()->print_error(output_prefix, "Error while accepting client connection ");
        return NULL;
    }

    if (!is_alive)
        return NULL;

    // Affichage de la connexion
    Output::GetInstance()->print(output_prefix, " [+] New connection from ", inet_ntoa(client_address.sin_addr), "\n");

    return client_socket;
}
#else
int EndPoint::open_socket()
{
    struct sockaddr_in address;
    int yes = 1;

    // Ouverture de la socket de connexion
    if ((socket_ = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while creating connection socket ");
        return false;
    }

    // Configuration de la socket de connexion
    if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while configuring connection socket ");
        return false;
    }

    // Configuration de l'adresse de transport        
    address.sin_addr.s_addr = INADDR_ANY;      // adresse, devrait être converti en reseau mais est egal à 0
    address.sin_family = AF_INET;              // type de la socket
    address.sin_port = htons(connection_port); // port, converti en reseau
    bzero(&(address.sin_zero), 8);             // mise a zero du reste de la structure

    // Demarrage du point de connexion : on ajoute l'adresse de transport dans la socket
    if (bind(socket_, (struct sockaddr*)&address, sizeof(struct sockaddr)) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while binding connection socket ");
        return false;
    }

    // Attente sur le point de connexion
    if (listen(socket_, BACKLOG) == -1) {
        Output::GetInstance()->print_error(output_prefix, "Error while listening connection socket ");
        return false;
    }

    return true;
}

int EndPoint::accept_connection()
{
    int client_socket;
    struct sockaddr_in client_address;
    unsigned int sinsize = sizeof(struct sockaddr_in);

    // Acceptation de la connexion
    if ((client_socket = accept(socket_, (struct sockaddr*)&client_address, &sinsize)) == -1) {
        if (!is_alive)
            return NULL;
        Output::GetInstance()->print_error(output_prefix, "Error while accepting client connection ");
        return NULL;
    }

    if (!is_alive)
        return NULL;

    // Affichage de la connexion
    Output::GetInstance()->print(output_prefix, " [+] New connection from ", inet_ntoa(client_address.sin_addr), "\n");

    return client_socket;
}
#endif

void EndPoint::end_thread()
{
    if (!is_alive)
        return;

    // End all clients
    for (std::vector<Client*>::iterator it = clients.begin(); it != clients.end(); ++it) {
        (*it)->end_thread();
    }

    ThreadedSocket::end_thread();
}

std::vector<Client*> EndPoint::getClients() const
{
    return clients;
}

void EndPoint::execute_thread()
{
    Output::GetInstance()->print(output_prefix, "Thread server starts.\n");

    // Ouverture de la socket de connexion
    Output::GetInstance()->print(output_prefix, "Trying to open connection socket on the port ", connection_port, "...\n");
    if (!open_socket()) {
        exit(EXIT_FAILURE);
    }
    Output::GetInstance()->print(output_prefix, "Connection socket opened successfully !\n");

    // Boucle infinie pour le serveur (pour accepter les connexions entrantes)
    int threads_count = 0;
    Client* c;
    while (1)
    {
        if (!is_alive)
            return;

        Output::GetInstance()->print(output_prefix, "Waiting for client connection...\n");

        threads_count++;
#ifdef _WIN32
        SOCKET client_socket = accept_connection();
#else
        int client_socket = accept_connection();
#endif
        if (!is_alive)
            return;

        if (client_socket != NULL) {
            c = new Client(threads_count, client_socket, MAXDATASIZE);
            if (!is_alive) {
                c->~Client();
                return;
            }

            c->start_thread();
            clients.push_back(c);
        }
    }
}
