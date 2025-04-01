#include <iostream>
#include <vector>
#include <cstring>
#include <gmpxx.h>

class AES {
private:
    // AES-256 parameters
    static const int Nb = 4;        // Number of columns in state (fixed at 4)
    static const int Nk = 8;        // Number of 32-bit words in key (8 for AES-256)
    static const int Nr = 14;       // Number of rounds (14 for AES-256)
    
    // State and key arrays
    uint8_t state[4][Nb];           // State array
    uint8_t roundKeys[4 * Nb * (Nr + 1)]; // Expanded key
    
    // AES S-box for SubBytes operation
    static const uint8_t sbox[256];
    // AES inverse S-box for InvSubBytes
    static const uint8_t rsbox[256];
    // Round constant for key expansion
    static const uint8_t rcon[11];
    
    // AES operations
    void subBytes();
    void shiftRows();
    void mixColumns();
    void addRoundKey(int round);
    
    // Key expansion
    void keyExpansion(const uint8_t* key);
    void rotWord(uint8_t* word);
    void subWord(uint8_t* word);
    
    // Helper function to convert state array to/from block
    void blockToState(const uint8_t* block);
    void stateToBlock(uint8_t* block);
    
public:
    //AES() {};
    // Constructor takes a 256-bit key
    void setKey(const uint8_t* key);
    
    // Encrypt a single 128-bit block
    void encryptBlock(const uint8_t* plaintext, uint8_t* ciphertext);
};