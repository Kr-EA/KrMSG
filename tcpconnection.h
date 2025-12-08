#pragma once

#include <cstdint>
#include <string>
#include <map>
#include <vector>
#include <QString>

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
#include <QString>

using socket_t = int;
#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif
#endif

struct Msg{
    int type;
    std::string msg;
};

class TCPConnection
{
public:
    TCPConnection(const char *HOST, const char *PORT, int BUFFER_SIZE);
    ~TCPConnection();

    void sendMessage(const std::vector<uint8_t> &data);
    Msg receiveMessage();
    void closeConnection();

    std::map<std::string, int> clientsEnumeration;

    std::vector<std::string> messageFragmentation(std::string message, int packet_size);
    void sendFile(QString file, QString prefix, int reciever, int fragment_size);

    bool isConnected() const { return sock != INVALID_SOCKET; }

private:
    socket_t sock{INVALID_SOCKET};
    int buffer_size{0};
    static bool recvAll(socket_t s, uint8_t *buf, size_t len);
    static bool sendAll(int sock, const void* data, size_t len);
};
