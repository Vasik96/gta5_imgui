#include "ExternalLogger.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

namespace ExternalLogger
{
    std::string serverIP;
    int serverPort;

    void InitializeExternalLogger(const std::string& ip, int port)
    {
        serverIP = ip;
        serverPort = port;
    }

    void LogExternal(const std::string& message)
    {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);

        SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (clientSocket == INVALID_SOCKET) return;

        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);
        serverAddr.sin_port = htons(serverPort);

        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        {
            closesocket(clientSocket);
            WSACleanup();
            return;
        }

        send(clientSocket, message.c_str(), static_cast<int>(message.size()), 0);
        closesocket(clientSocket);
        WSACleanup();
    }
}
