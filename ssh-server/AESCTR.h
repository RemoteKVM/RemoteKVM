#ifndef AESCTR_H
#define AESCTR_H

#include "AES.h"
#include <gmpxx.h>
#include <cstring>

// CTR mode implementation
class AESCTR {
private:
    AES aes;
    uint8_t counter[16];
    
    void incrementCounter();
    
public:
    void setKeyAndIV(const uint8_t* key, const uint8_t* iv);
    void process(const uint8_t* input, uint8_t* output, size_t length);
    mpz_class getCounter() const;
    void setCounter(const mpz_class& value);
};

#endif // AESCTR_H