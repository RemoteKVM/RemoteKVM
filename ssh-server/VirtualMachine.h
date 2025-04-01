#pragma once

#include <string>

struct VirtualMachine {
    int id = -1;
    std::string name;
    std::string status;
    int storageSize;
};