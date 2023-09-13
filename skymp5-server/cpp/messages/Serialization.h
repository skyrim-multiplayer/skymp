#pragma once
#include <vector>
#include <cstdint>

// TODO
class Serialization {
public:
    static void SerializeMessage(const char *jsonContent, std::vector<uint8_t> &outputBuffer);
};
