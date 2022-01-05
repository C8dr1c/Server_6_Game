#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>

#ifdef _WIN32
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
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

using namespace std;

// Valeurs constantes utilisées dans le programme
// ----------------------------------------------------------------------------------------------------------------

// Nombre maximal d'octets à envoyer ou à lire en une fois
#define MAXDATASIZE 100 


// Prototypes des fonctions disponibles dans le programme
// (l'implémentation est faite dans ce fichier)
// ----------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

SOCKET open_connection(char*, int);
bool close_connection(SOCKET);

#else

int open_connection(char*, int);
bool close_connection(int);

#endif

bool strtoi(char*, int*);


// Fonctions
// ----------------------------------------------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    cout << "*********************************************************" << endl;
    cout << "*           Simple socket client application            *" << endl;
    cout << "*********************************************************" << endl;
    cout << endl;
    cout << endl;

    // Lecture des paramètres en cours
    int port;
    if (argc != 3 || !strtoi(argv[2], &port)) {
        cout << "Invalid parameters !" << endl;
        cout << "program usage : " << argv[0] << " server_address connection_port" << endl;
        exit(EXIT_FAILURE);
    }
    cout << "Trying to open connection to server " << argv[1] << " on the port " << port << "..." << endl;

#ifdef _WIN32
    SOCKET s;
#else
    int s;
#endif
    s = open_connection(argv[1], port);
    cout << "Connection to server opened successfully !" << endl;

    char buffer[MAXDATASIZE];
    int length;

    memset(buffer, 0, MAXDATASIZE * sizeof(char));

    while (1)
    {
        cout << endl << argv[1] << "> ";
        cin >> buffer;

        if (strcmp(buffer, "EXIT") == 0)
            break;

        // Envoi du buffer au serveur
        if (send(s, buffer, strlen(buffer), 0) == -1) {
            perror("Error while sending message to server ");
        }

        // Lecture de la réponse du serveur
        if ((length = recv(s, buffer, MAXDATASIZE, 0)) == -1) {
            perror("Error while receiving message from server ");
            continue;
        }

        // Suppression des retours chariots (\n et \r)
        while (length > 0 && (buffer[length - 1] == '\n' || buffer[length - 1] == '\r'))
            length--;
        // Ajout de backslash zero a la fin pour en faire une chaine de caracteres
        if (length >= 0 && length < MAXDATASIZE)
            buffer[length] = '\0';

        if (strcmp(buffer, "CONNECTION_CLOSED") == 0) {
            cout << "Le serveur a ferme la connexion !" << endl;
            break;
        }
        else {
            // Affichage du message
            cout << buffer << endl;
        }
        
    }

    close_connection(s);

    return(EXIT_SUCCESS);
}


#ifdef _WIN32

SOCKET open_connection(char* server_address, int connection_port)
{
    SOCKET s;
    SOCKADDR_IN saddress;

    // Initialisation l'utilisation des WinSocks par un processus
    WSADATA WSAData;
    if (WSAStartup(MAKEWORD(2, 0), &WSAData) == -1) {
        perror("Error while starting using WinSocks ");
        exit(EXIT_FAILURE);
    }

    // Ouverture de la socket
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating socket ");
        exit(EXIT_FAILURE);
    }

    // Récupération de l'adresse IP du serveur
    if ((saddress.sin_addr.s_addr = inet_addr(server_address)) == INADDR_NONE) {
        fprintf(stderr, "Server ip address is invalid !\n");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse de transport        
    saddress.sin_family = AF_INET;               // type de la socket
    saddress.sin_port = htons(connection_port);  // port, converti en réseau

    // Tentative de connexion sur le point de connexion du serveur
    if (connect(s, (struct sockaddr*)&saddress, sizeof(struct sockaddr)) == -1) {
        perror("Error while connecting to server ");
        exit(EXIT_FAILURE);
    }

    return s;
}

bool close_connection(SOCKET s)
{
    if (closesocket(s) == -1) {
        perror("Error while closing socket ");
        return false;
    }
    if (WSACleanup() == -1) {
        perror("Error while cleaning WinSocks ");
        return false;
    }
    return true;
}

#else

int open_connection(char* server_address, int connection_port)
{
    int s;
    struct sockaddr_in saddress;
    int yes = 1;

    // Ouverture de la socket de connexion
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error while creating connection socket ");
        exit(EXIT_FAILURE);
    }

    // Configuration de la socket de connexion
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("Error while configuring connection socket ");
        exit(EXIT_FAILURE);
    }

    // Récupération de l'adresse IP du serveur
    if ((saddress.sin_addr.s_addr = inet_addr(server_address)) == INADDR_NONE) {
        fprintf(stderr, "Server ip address is invalid !\n");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse de transport        
    saddress.sin_family = AF_INET;               // type de la socket
    saddress.sin_port = htons(connection_port);  // port, converti en réseau
    bzero(&(address.sin_zero), 8);               // mise a zero du reste de la structure

    // Tentative de connexion sur le point de connexion du serveur
    if (connect(s, (struct sockaddr*)&saddress, sizeof(struct sockaddr)) == -1) {
        perror("Error while connecting to server ");
        exit(EXIT_FAILURE);
    }

    return s;
}

bool close_connection(int s)
{
    if (close(s) == -1) {
        perror("Error while closing socket ");
        return false;
    }
    return true;
}

#endif

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