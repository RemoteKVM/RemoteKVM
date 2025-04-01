#pragma once
#include <iostream>
#include <vector>

class SHA256 
{
public:
    static std::vector<uint8_t> hash(std::vector<uint8_t> input);

private:
    static void padBlock(std::vector<uint8_t>& block);
    static std::vector<uint32_t> scheduleMessage(const std::vector<uint8_t>& block);
    static void compress(std::vector<uint32_t>& state, const std::vector<uint32_t>& words);


    static inline uint32_t rightRotate(uint32_t value, uint32_t bits);
};
    