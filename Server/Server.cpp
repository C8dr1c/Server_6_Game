/*
#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <thread>
#include <mutex>
#include <cstdarg>

#ifdef _WIN32

#include <winsock2.h>
#include <Windows.h>
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

#include "Semaphore.h"


// Valeurs constantes utilisées dans le programme
// ----------------------------------------------------------------------------------------------------------------

// Nombre maximal de connexions en attente
#define BACKLOG 10

// Nombre maximal d'octets à envoyer ou à lire en une fois
#define MAXDATASIZE 100


// Prototypes des fonctions disponibles dans le programme
// (l'implémentation est faite dans ce fichier)
// ----------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

SOCKET open_connection(int);
SOCKET accept_connection(SOCKET);
bool close_connection(SOCKET, bool);
void start_client(SOCKET, int);

void sleep(DWORD);
void usleep(DWORD);

#else

int open_connection(int);
int accept_connection(int);
bool close_connection(int, bool);
void start_client(int, int);

#endif

bool strtoi(char*, int*);
template <typename... T> void print(T...);


// Variables globales
// ----------------------------------------------------------------------------------------------------------------

// Sémaphore pour réserver l'accès à la sortie standard
// Géré dans print()
CSemaphore sem_std_out(1);


// Fonctions
// ----------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    std::cout << "*********************************************************" << std::endl;
    std::cout << "*           Simple socket server application            *" << std::endl;
    std::cout << "*********************************************************" << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;

    // Lecture des paramètres en cours
    int port;
    if (argc != 2 || !strtoi(argv[1], &port)) {
        std::cout << "Invalid parameters !" << std::endl;
        std::cout << "program usage : " << argv[0] << " connection_port" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Trying to open connection socket on the port " << port << "..." << std::endl;

#ifdef _WIN32
    SOCKET connection_socket;
#else
    int connection_socket;
#endif

    // Ouverture de la socket de connexion
    connection_socket = open_connection(port);
    std::cout << "Connection socket opened successfully !" << std::endl;

    // Boucle pour accepter les connexions entrantes
    int threads_count = 0;
    while (1)
    {
        print("\nWaiting for client connection...\n");

#ifdef _WIN32
        SOCKET client_socket;
#else
        int client_socket;
#endif

        // Attente de la connexion client
        client_socket = accept_connection(connection_socket);
        if (client_socket == NULL)
            continue;

        // Démarrage du thread dédié au client
        threads_count++;
        std::thread t_client(start_client, client_socket, threads_count);
        t_client.detach();
    }

    print("Trying to clse connection socket...\n");
    if (close_connection(connection_socket, true)) {
        print("Connection socket closed successfully !\n");
        return(EXIT_SUCCESS);
    };

    return(EXIT_FAILURE);
}

#ifdef _WIN32

SOCKET open_connection(int connection_port)
{
    SOCKET connection_socket;
    SOCKADDR_IN address;

    // Initialisation l'utilisation des WinSocks par un processus
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 0), &WSAData) == -1) {
        perror("Error while starting using WinSocks : ");
        exit(EXIT_FAILURE);
    }

    // Ouverture de la socket de connexion
    if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating connection socket : ");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse de transport
    address.sin_addr.s_addr = INADDR_ANY;      // adresse, devrait être converti en reseau mais est egal à 0
    address.sin_family = AF_INET;              // type de la socket
    address.sin_port = htons(connection_port); // port, converti en reseau

    // Demarrage du point de connexion : on ajoute l'adresse de transport dans la socket
    if (bind(connection_socket, (SOCKADDR*)&address, sizeof(address)) == -1) {
        perror("Error while binding connection socket : ");
        exit(EXIT_FAILURE);
    }

    // Attente sur le point de connexion
    if (listen(connection_socket, BACKLOG) == -1) {
        perror("Error while listening connection socket : ");
        exit(EXIT_FAILURE);
    }

    return connection_socket;
}

SOCKET accept_connection(SOCKET connection_socket) 
{
    SOCKET client_socket;
    SOCKADDR_IN caddress;
    int sinsize = sizeof(caddress);

    // Acceptation de la connexion
    if ((client_socket = accept(connection_socket, (SOCKADDR*)&caddress, &sinsize)) == -1) {
        perror("Error while accepting client connection : ");
        return NULL;
    }

    // Affichage de la connexion
    print("[+] New connection from ", inet_ntoa(caddress.sin_addr), "\n");

    return client_socket;
}

bool close_connection(SOCKET s, bool cleanWinsocks)
{
    if (closesocket(s) == -1) {
        perror("Error while closing socket : ");
        return false;
    }
    if (cleanWinsocks && WSACleanup() == -1) {
        perror("Error while cleaning WinSocks : ");
        return false;
    }
    return true;
}

void start_client(SOCKET s, int id)
{
    char buffer[MAXDATASIZE];         // Message recu
    int length;                       // Taille du message reçu
    time_t time_value;
    struct tm* time_info;

    print("[C", id, "] Thread client starts with id=", id, ".\n");

    // Boucle infinie pour le client
    while (1) {

        // On attend un message du client
        if ((length = recv(s, buffer, MAXDATASIZE, 0)) == -1)
        {
            char error[MAXDATASIZE];
            sprintf(error, "[C%d] Error while receiving message from client : ", id);
            perror(error);
            print("\n");
            break;
        }

        // Suppression des retours chariots (\n et \r)
        while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
            length--;
        // Ajout de backslash zero a la fin pour en faire une chaine de caracteres
        if (length >= 0 && length < MAXDATASIZE)
            buffer[length] = '\0';

        // Affichage du message
        print("[C", id, "] Message received : ", buffer, "\n");

        if (strcmp(buffer, "DISCONNECT") == 0) {
            break;
        }
        else {

            // On recupere l'heure et la date
            time(&time_value);
            time_info = localtime(&time_value);

            // Traitement du message reçu
            if (strcmp(buffer, "DATE") == 0)
                strftime(buffer, MAXDATASIZE, "%e/%m/%Y", time_info);
            else if (strcmp(buffer, "DAY") == 0)
                strftime(buffer, MAXDATASIZE, "%A", time_info);
            else if (strcmp(buffer, "MONTH") == 0)
                strftime(buffer, MAXDATASIZE, "%B", time_info);
            else
                sprintf(buffer, "%s is not recognized as a valid command", buffer);

            // On envoie le buffer
            print("[C", id, "] Sending message \"", buffer, "\" to client...\n");
            if (send(s, buffer, strlen(buffer), 0) == -1) {
                char error[MAXDATASIZE];
                sprintf(error, "[C%d] Error while sending message to client : ", id);
                perror(error);
                print("\n");
                break;
            }

            print("[C", id, "] Message \"", buffer, "\" send to client successfully.\n");
        }
    }

    print("[C", id, "] Closing client socket...\n");
    if (close_connection(s, false))
        print("[C", id, "] Client socket closed successfully.\n");

    print("[C", id, "] Thread client ends.\n");
}

void sleep(DWORD dwMilliseconds)
{
    Sleep(dwMilliseconds * 1000);
}
void usleep(DWORD dwMilliseconds)
{
    Sleep(dwMilliseconds);
}

#else

int open_connection(int connection_port)
{
    int connection_socket;
    struct sockaddr_in address;
    int yes = 1;

    // Ouverture de la socket de connexion
    if ((connection_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating connection socket : ");
        exit(EXIT_FAILURE);
    }

    // Configuration de la socket de connexion
    if (setsockopt(connection_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Error while configuring connection socket : ");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse de transport        
    address.sin_addr.s_addr = INADDR_ANY;      // adresse, devrait être converti en reseau mais est egal à 0
    address.sin_family = AF_INET;              // type de la socket
    address.sin_port = htons(connection_port); // port, converti en reseau
    bzero(&(address.sin_zero), 8);             // mise a zero du reste de la structure

    // Demarrage du point de connexion : on ajoute l'adresse de transport dans la socket
    if (bind(connection_socket, (struct sockaddr*)&address, sizeof(struct sockaddr)) == -1) {
        perror("Error while binding connection socket : ");
        exit(EXIT_FAILURE);
    }

    // Attente sur le point de connexion
    if (listen(connection_socket, BACKLOG) == -1) {
        perror("Error while listening connection socket : ");
        exit(EXIT_FAILURE);
    }

    return connection_socket;
}

int accept_connection(int s)
{
    int client_socket;
    struct sockaddr_in caddress;
    unsigned int sinsize = sizeof(struct sockaddr_in);

    // Acceptation de la connexion
    if ((client_socket = accept(connection_socket, (struct sockaddr*)&caddress, &sinsize)) == -1) {
        perror("Error while accepting client connection : ");
        return NULL;
    }

    // Affichage de la connexion
    print("[+] New connection from ", inet_ntoa(caddress.sin_addr), "\n");

    return client_socket;
}

bool close_connection(int s, bool notUsed)
{
    if (close(s) == -1) {
        perror("Error while closing socket : ");
        return false;
    }
    return true;
}

void start_client(int s, int id)
{
    char buffer[MAXDATASIZE];         // Message recu
    int length;                       // Taille du message reçu
    time_t time_value;
    struct tm* time_info;

    print("[C", id, "] Thread client starts with id=", id, ".\n");

    // Boucle infinie pour le client
    while (1) {

        // On attend un message du client
        if ((length = recv(s, buffer, MAXDATASIZE, 0)) == -1)
        {
            char error[MAXDATASIZE];
            sprintf(error, "[C%d] Error while receiving message from client : ", id);
            perror(error);
            print("\n");
            break;
        }

        // Suppression des retours chariots (\n et \r)
        while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
            length--;
        // Ajout de backslash zero a la fin pour en faire une chaine de caracteres
        if (length >= 0 && length < MAXDATASIZE)
            buffer[length] = '\0';

        // Affichage du message
        print("[C", id, "] Message received : ", buffer, "\n");

        if (strcmp(buffer, "DISCONNECT") == 0) {
            break;
        }
        else {

            // On recupere l'heure et la date
            time(&time_value);
            time_info = localtime(&time_value);

            // Traitement du message reçu
            if (strcmp(buffer, "DATE") == 0)
                strftime(buffer, MAXDATASIZE, "%e/%m/%Y", time_info);
            else if (strcmp(buffer, "DAY") == 0)
                strftime(buffer, MAXDATASIZE, "%A", time_info);
            else if (strcmp(buffer, "MONTH") == 0)
                strftime(buffer, MAXDATASIZE, "%B", time_info);
            else
                sprintf(buffer, "%s is not recognized as a valid command", buffer);

            // On envoie le buffer
            print("[C", id, "] Sending message \"", buffer, "\" to client...\n");
            if (send(s, buffer, strlen(buffer), 0) == -1) {
                char error[MAXDATASIZE];
                sprintf(error, "[C%d] Error while sending message to client : ", id);
                perror(error);
                print("\n");
                break;
            }

            print("[C", id, "] Message \"", buffer, "\" send to client successfully.\n");
        }
    }

    print("[C", id, "] Closing client socket...\n");
    if (close_connection(s, false))
        print("[C", id, "] Client socket closed successfully.\n");

    print("[C", id, "] Thread client ends.\n");
}

#endif

template <typename... T> void print(T... args)
{
    sem_std_out.wait();
    ((std::cout << args), ...);
    std::cout.flush();
    sem_std_out.notify();
}

bool strtoi(char* value, int* result)
{
    char* p;
    errno = 0;
    long arg = strtol(value, &p, 10);
    if (*p != '\0' || errno != 0) {
        return false;
    }

    if (arg < INT_MIN || arg > INT_MAX) {
        return false;
    }

    *result = arg;
    return true;
}
*/