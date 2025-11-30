#include "tcpconnection.h"
#include <sstream>

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

std::string TCPConnection::receiveMessage()
{
    if (sock == INVALID_SOCKET)
        return "";

    uint8_t header[3];
    if (!recvAll(sock, header, 3)) {
        return "";
    }

    uint8_t type = header[0];
    (void) type;

    uint16_t length = (static_cast<uint16_t>(header[1]) << 8) | static_cast<uint16_t>(header[2]);

    if (length > 65535) {
        std::cerr << "Invalid length: " << length << "\n";
        return "";
    }

    std::vector<uint8_t> data(length);
    if (length > 0 && !recvAll(sock, data.data(), length)) {
        return "";
    }

    std::string result;
    result.reserve(length);
    for (uint8_t ch : data) {
        result.push_back(static_cast<char>(ch));
    }

    return result;
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
