#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include "LoggingServer.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 5000

void LoggingServer::StartLogServer() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    int addrLen = sizeof(clientAddr);

    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    std::cout << "Log server started on port " << PORT << std::endl;

    while (true) {
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) continue;

        char buffer[1024] = { 0 };
        recv(clientSocket, buffer, sizeof(buffer), 0);

        std::string receivedLog(buffer);
        int logLevel = 0; // You can modify this if you want to send levels in the future
        Logging::Log(receivedLog, 4);  // Calls the actual Logging system

        closesocket(clientSocket);
    }

    closesocket(serverSocket);
    WSACleanup();
}
