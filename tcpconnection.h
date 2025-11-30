#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

using socket_t = SOCKET;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (SOCKET)(~0)
#endif
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

using socket_t = int;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

class TCPConnection
{
public:
    TCPConnection(const char *HOST, const char *PORT, int BUFFER_SIZE);
    ~TCPConnection();

    void sendMessage(const std::vector<uint8_t> &data);
    std::string receiveMessage();
    void closeConnection();

    std::vector<std::string> messageFragmentation(std::string message, int packet_size);

    bool isConnected() const { return sock != INVALID_SOCKET; }

private:
    socket_t sock{INVALID_SOCKET};
    int buffer_size{0};

    static bool recvAll(socket_t s, uint8_t *buf, size_t len);
};
