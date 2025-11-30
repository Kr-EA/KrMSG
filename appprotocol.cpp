#include <cstdint>
#include <vector>
#include "appprotocol.h"

using namespace std;

AppProtocol::AppProtocol(int type, const std::string& data){
    code.push_back(type);
    code.push_back(static_cast<uint8_t>(data.size() >> 8));
    code.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    code.insert(code.end(), data.begin(), data.end());
};

AppProtocol::AppProtocol(int type, std::vector<uint8_t> data){
    code.push_back(type);
    code.push_back(static_cast<uint8_t>(data.size() >> 8));
    code.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    code.insert(code.end(), data.begin(), data.end());
};

vector<uint8_t> AppProtocol::getCode(){
    return code;
}

Packet AppProtocol::getPacket(){
    return packet;
}
