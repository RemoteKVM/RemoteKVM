#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <random>
#include "SshTypeHelper.h"

class SshPacket {
public:
    // Serialize the packet into a vector of bytes following the SSH binary packet format.
    static std::vector<uint8_t> serialize(std::vector<uint8_t> payload);
};
