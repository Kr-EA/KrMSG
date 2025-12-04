#include <cstdint>
#include <vector>
#include "AppProtocol.h"

using namespace std;

AppProtocol::AppProtocol(int type, int reciever, const std::string& data){
    code.push_back(type);
    code.push_back(static_cast<uint8_t>(data.size() >> 8));
    code.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    code.push_back(reciever);
    code.insert(code.end(), data.begin(), data.end());
};

AppProtocol::AppProtocol(int type, int reciever, std::vector<uint8_t> data){
    code.push_back(type);
    code.push_back(static_cast<uint8_t>(data.size() >> 8));
    code.push_back(static_cast<uint8_t>(data.size() & 0xFF));
    code.push_back(reciever);
    code.insert(code.end(), data.begin(), data.end());
};

vector<uint8_t> AppProtocol::getCode(){
    return code;
}
