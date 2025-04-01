#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <gmpxx.h> // For mpz_class 
#include "SHA256.h"
#include "SshTypeHelper.h"
#include "SshException.h"

class RSA {
public:
    RSA();
    ~RSA();

    void loadPrivateKeyFromPEM(const std::string& filename);
    std::vector<uint8_t> getPublicKeyBlob();
    std::vector<uint8_t> getSSHSignature(const std::vector<uint8_t>& data);

private:
    bool initialized;
    mpz_class n, e, d, p, q;

    std::vector<uint8_t> base64Decode(const std::string& input);
    void parseDERPrivateKey(const std::vector<uint8_t>& der);
    size_t extractDerInteger(const std::vector<uint8_t>& der, size_t index, mpz_class& value);
    std::vector<uint8_t> createDigestInfo(const std::vector<uint8_t>& hash);
    std::vector<uint8_t> applyPKCS1Padding(const std::vector<uint8_t>& t, size_t k);
    std::vector<uint8_t> mpzToBytes(const mpz_class& value, size_t desired_length = 0);
    mpz_class bytesToMpz(const std::vector<uint8_t>& bytes);
};