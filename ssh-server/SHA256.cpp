#include "SHA256.h"

#define BLOCK_SIZE 512
#define BITS_IN_BYTE 8
#define WORDS_IN_BLOCK 64

// just some math nerds that figured these out
const uint32_t k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::vector<uint8_t> SHA256::hash(std::vector<uint8_t> input)
{
    padBlock(input);

    // initialize hash state variables (some math nerds chose these as well)
    std::vector<uint32_t> state = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // process each 512-bit (64-byte) block
    for (size_t i = 0; i < input.size(); i += 64)
    {
        std::vector<uint8_t> block(input.begin() + i, input.begin() + i + 64);
        std::vector<uint32_t> words = scheduleMessage(block);

        compress(state, words);
    }

    // convert final hash state to byte array
    std::vector<uint8_t> hashBytes;
    for (uint32_t value : state)
    {
        hashBytes.push_back((value >> 24) & 0xFF);
        hashBytes.push_back((value >> 16) & 0xFF);
        hashBytes.push_back((value >> 8) & 0xFF);
        hashBytes.push_back(value & 0xFF);
    }

    return hashBytes;
}

void SHA256::padBlock(std::vector<uint8_t>& block)
{
    uint64_t inputBinaryLen = block.size() * BITS_IN_BYTE;
    uint64_t paddingSizeInBits = (BLOCK_SIZE - inputBinaryLen - 64) % BLOCK_SIZE;
    uint64_t paddingSizeInBytes = paddingSizeInBits / BITS_IN_BYTE - 1; // minus one because we manually pad the 1

    block.emplace_back(0b10000000);
    block.insert(block.end(), paddingSizeInBytes, 0x00);

    // append size of message
    for (int i = sizeof(uint64_t) - 1; i >= 0; i--)
    {
        block.emplace_back(static_cast<uint8_t>((inputBinaryLen >> (i * 8)) & 0xFF));
    }
}

uint32_t SHA256::rightRotate(uint32_t value, uint32_t bits) 
{
    return (value >> bits) | (value << (32 - bits));
}


std::vector<uint32_t> SHA256::scheduleMessage(const std::vector<uint8_t>& block)
{
    std::vector<uint32_t> w(WORDS_IN_BLOCK);

    for (size_t i = 0; i < 16; ++i)
    {
        w[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | (block[i * 4 + 3]);
    }

    for (size_t i = 16; i < WORDS_IN_BLOCK; ++i)
    {
        uint32_t s0 = rightRotate(w[i - 15], 7) ^ rightRotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = rightRotate(w[i - 2], 17) ^ rightRotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    return w;
}

void SHA256::compress(std::vector<uint32_t>& state, const std::vector<uint32_t>& words)
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (size_t i = 0; i < WORDS_IN_BLOCK; ++i)
    {
        uint32_t S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t temp1 = h + S1 + ch + k[i] + words[i];
        uint32_t S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state[0] += a; 
    state[1] += b; 
    state[2] += c;
    state[3] += d;
    state[4] += e; 
    state[5] += f; 
    state[6] += g; 
    state[7] += h;
}