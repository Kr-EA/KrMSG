#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct Packet {
    int type;
    int size;
    std::string data;
};

class AppProtocol{
private:
    std::vector<uint8_t> code;
    Packet packet;
public:
    AppProtocol(int, const std::string& data);
    AppProtocol(int, std::vector<uint8_t>);
    std::vector<uint8_t> getCode();
    Packet getPacket();
};
