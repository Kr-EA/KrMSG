#include "tcpconnection.h"
#include <iostream>
#include <sstream>
#include <QFileInfo>
#include <fstream>
#include "appprotocol.h"

std::vector<std::string> splitString(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::stringstream ss(s);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

TCPConnection::TCPConnection(const char *HOST, const char *PORT, const int BUFFER_SIZE)
{
    buffer_size = BUFFER_SIZE;
    sock = INVALID_SOCKET;

#ifdef _WIN32
    WSADATA wsaData;
    int wsaerr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaerr != 0) {
        std::cerr << "WSAStartup failed: " << wsaerr << std::endl;
        return;
    }
#endif

    struct addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;

    int status = getaddrinfo(HOST, PORT, &hints, &res);
    if (status != 0) {
        std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
        sock = INVALID_SOCKET;
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    for (struct addrinfo *p = res; p != nullptr; p = p->ai_next) {
        socket_t s = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (s == INVALID_SOCKET) {
            continue;
        }

        if (::connect(s, p->ai_addr, p->ai_addrlen) == -1) {
#ifdef _WIN32
            closesocket(s);
#else
            close(s);
#endif
            continue;
        }

        sock = s;
        break;
    }

    freeaddrinfo(res);

    if (sock == INVALID_SOCKET) {
        std::cerr << "Error while creating a socket" << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
    }
}

void TCPConnection::sendFile(QString file, QString prefix, int reciever, int fragment_size){

    QFileInfo info(file);

    AppProtocol startFile(10, reciever, (prefix+'_'+info.fileName()).toStdString());
    std::vector<uint8_t> packet = startFile.getCode();
    sendMessage(packet);

    std::ifstream filestream(file.toStdString(), std::ios::binary);

    std::vector<uint8_t> buffer(fragment_size);

    while (true) {
        filestream.read(reinterpret_cast<char*>(buffer.data()), buffer.size());
        std::streamsize bytesRead = filestream.gcount();

        if (bytesRead <= 0)
            break;

        AppProtocol partFile(11, reciever, buffer);
        packet = partFile.getCode();
        sendMessage(packet);

        if (bytesRead < static_cast<std::streamsize>(fragment_size))
            break;
    }
    filestream.close();

    AppProtocol endFile(12, reciever, (prefix+'_'+info.fileName()).toStdString());
    packet = endFile.getCode();
    sendMessage(packet);
}

void TCPConnection::sendMessage(const std::vector<uint8_t> &data)
{
    if (sock == INVALID_SOCKET)
        return;

    const char *ptr = reinterpret_cast<const char *>(data.data());
    size_t total = 0;
    size_t len = data.size();

    while (total < len) {
        int sent = ::send(sock, ptr + total, static_cast<int>(len - total), 0);
        if (sent <= 0) {
            std::cerr << "send error" << std::endl;
            break;
        }
        total += sent;
    }
}

bool TCPConnection::recvAll(socket_t s, uint8_t *buf, size_t len)
{
    size_t total = 0;

    while (total < len) {
        int received = recv(s,
                            reinterpret_cast<char *>(buf + total),
                            static_cast<int>(len - total),
                            0);

        if (received <= 0) {
            return false;
        }

        total += received;
    }

    return true;
}

bool TCPConnection::sendAll(int sock, const void* data, size_t len) {
    const char* ptr = static_cast<const char*>(data);
    size_t totalSent = 0;

    while (totalSent < len) {
        int sent = ::send(sock, ptr + totalSent, len - totalSent, 0);
        if (sent <= 0) {
            return false;
        }
        totalSent += sent;
    }
    return true;
}

Msg TCPConnection::receiveMessage()
{
    Msg msg;
    if (sock == -1) return msg;

    uint8_t header[4];
    if (!recvAll(sock, header, 4)) {
        return msg;
    }

    uint8_t type = header[0];
    uint16_t length = (static_cast<uint16_t>(header[1]) << 8) | header[2];
    uint8_t reciever = header[3];

    msg.type = type;

    if (length > 65535) {
        std::cerr << "Invalid length: " << length << "\n";
        return msg;
    }

    std::vector<uint8_t> data(length);
    if (length > 0 && !recvAll(sock, data.data(), length)) {
        return msg;
    }

    std::string result;
    for (uint8_t ch : data){
        result += ch;
    }

    if (type == 4){
        clientsEnumeration.erase(result);
        msg.msg = "clientSYNC";
        return msg;
    }

    if (type == 3){
        clientsEnumeration[result] = reciever;
        msg.msg = "clientSYNC";
        return msg;
    }

    else msg.msg = result;

    return msg;
}

void TCPConnection::closeConnection()
{
    if (sock != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(sock);
        WSACleanup();
#else
        close(sock);
#endif
        sock = INVALID_SOCKET;
    }
}

std::vector<std::string> TCPConnection::messageFragmentation(std::string message, int packet_size)
{
    std::vector<std::string> words = splitString(message, ' ');
    std::vector<std::string> packets{};
    std::string packet{};
    for (std::string word : words) {
        if (packet.size() + word.size() + 1 < packet_size) {
            word.append(" ");
            packet.append(word);
        } else {
            packets.push_back(packet);
            packet.clear();
            word.append(" ");
            packet.append(word);
        }
    }
    if (!packet.empty()) {
        packets.push_back(packet);
    }
    return packets;
}

TCPConnection::~TCPConnection()
{
    closeConnection();
}
