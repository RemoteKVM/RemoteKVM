#include "AESCTR.h"

void AESCTR::incrementCounter() {
    // Increment counter as a 128-bit big-endian integer
    for (int i = 15; i >= 0; i--) {
        if (++counter[i] != 0) {
            break;
        }
    }
}

void AESCTR::setKeyAndIV(const uint8_t* key, const uint8_t* iv) {
    aes.setKey(key);
    memcpy(counter, iv, 16);
}

void AESCTR::process(const uint8_t* input, uint8_t* output, size_t length) {
    uint8_t keystream[16];
    size_t processed = 0;
    
    while (processed < length) {
        // Encrypt counter to get keystream
        aes.encryptBlock(counter, keystream);
        
        // XOR keystream with input to get output
        size_t blockSize = std::min(size_t(16), length - processed);
        for (size_t i = 0; i < blockSize; i++) {
            output[processed + i] = input[processed + i] ^ keystream[i];
        }
        
        // Increment counter for next block
        incrementCounter();
        processed += blockSize;
    }
}

mpz_class AESCTR::getCounter() const {
    mpz_class result;
    mpz_import(result.get_mpz_t(), 16, 1, 1, 1, 0, counter);
    return result;
}

void AESCTR::setCounter(const mpz_class& value) {
    mpz_export(counter, nullptr, 1, 16, 1, 0, value.get_mpz_t());
}