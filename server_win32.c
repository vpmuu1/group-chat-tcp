//define _WINSOCK_DEPRECATED_NO_WARNINGS;_CRT_SECURE_NO_WARNINGS in proj

#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define INET_ADDRSTRLEN 16
#pragma comment(lib, "ws2_32.lib") 

struct client_info {
    SOCKET sockno;
    char ip[INET_ADDRSTRLEN];
};

SOCKET clients[100];
int n = 0;
CRITICAL_SECTION cs;

void sendToAll(char* msg, SOCKET curr) {
    int i;
    EnterCriticalSection(&cs);
    for (i = 0; i < n; i++) {
        if (clients[i] != curr) {
            if (send(clients[i], msg, strlen(msg), 0) < 0) {
                perror("sending failure...");
                continue;
            }
        }
    }
    LeaveCriticalSection(&cs);
}

DWORD WINAPI recvMsg(LPVOID sock) {
    struct client_info cl = *((struct client_info*)sock);
    char msg[500];
    int len;

    while ((len = recv(cl.sockno, msg, 500, 0)) > 0) {
        msg[len] = '\0';
        sendToAll(msg, cl.sockno);
        memset(msg, '\0', sizeof(msg));
    }

    EnterCriticalSection(&cs);
    printf("%s disconnected...\n", cl.ip);

    int i, j;
    for (i = 0; i < n; i++) {
        if (clients[i] == cl.sockno) {
            j = i;
            while (j < (n - 1)) {
                clients[j] = clients[j + 1];
                j++;
            }
        }
    }
    n--;
    LeaveCriticalSection(&cs);
    return 0;
}

int main(int argc, char* argv[]) {
    WSADATA wsaData;
    struct sockaddr_in my_addr, their_addr;
    struct client_info cl;
    int portno;
    SOCKET my_sock, their_sock;
    int their_addr_size;

    if (argc != 2) {
        perror("usage: server <port>");
        exit(1);
    }

    portno = atoi(argv[1]);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        perror("WSAStartup failed");
        exit(1);
    }

    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock == INVALID_SOCKET) {
        perror("socket failed");
        WSACleanup();
        exit(1);
    }


    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portno);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    my_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    their_addr_size = sizeof(their_addr);

    if (bind(my_sock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR) {
        perror("bind failed");
        closesocket(my_sock);
        WSACleanup();
        exit(1);
    }

    if (listen(my_sock, 5) == SOCKET_ERROR) {
        perror("listen failed");
        closesocket(my_sock);
        WSACleanup();
        exit(1);
    }

    InitializeCriticalSection(&cs);

    while (1) {
        their_sock = accept(my_sock, (struct sockaddr*)&their_addr, &their_addr_size);
        if (their_sock == INVALID_SOCKET) {
            perror("accept failed");
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        //inet_ntop(AF_INET, (struct sockaddr*)&their_addr, ip, INET_ADDRSTRLEN);
        printf("%s connected...\n", ip);

        cl.sockno = their_sock;
        strcpy(cl.ip, ip);
        clients[n] = their_sock;
        n++;

        CreateThread(NULL, 0, recvMsg, (LPVOID)&cl, 0, NULL);
    }

    closesocket(my_sock);
    WSACleanup();
    return 0;
}
