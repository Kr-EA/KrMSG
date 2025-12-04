#pragma once
#include <vector>
#include <string>
#include <cstdint>

class AppProtocol{
private:
    std::vector<uint8_t> code;
public:
    AppProtocol(int, int, const std::string& data);
    AppProtocol(int, int, std::vector<uint8_t>);
    std::vector<uint8_t> getCode();
};
