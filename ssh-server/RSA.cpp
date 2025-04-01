#include "RSA.h"

RSA::RSA() : initialized(false) {}
RSA::~RSA() {}

void RSA::loadPrivateKeyFromPEM(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw SshException("Unable to open key file: " + filename);
    }

    std::string line, pemContent;
    bool inKey = false;

    while (std::getline(file, line)) {
        if (line == "-----BEGIN RSA PRIVATE KEY-----") {
            inKey = true;
        } else if (line == "-----END RSA PRIVATE KEY-----") {
            inKey = false;
            break;
        } else if (inKey) {
            pemContent += line;
        }
    }

    file.close();
    if (pemContent.empty()) {
        throw SshException("No private key found in file");
    }

    std::vector<uint8_t> derData = base64Decode(pemContent);
    parseDERPrivateKey(derData);
    initialized = true;
}

std::vector<uint8_t> RSA::getPublicKeyBlob() {
    std::vector<uint8_t> result;

    SshTypeHelper::appendAsSshStringToVector(result, "ssh-rsa");
    SshTypeHelper::appendAsMpintToVector(result, e);
    SshTypeHelper::appendAsMpintToVector(result, n);

    return result;
}

std::vector<uint8_t> RSA::getSSHSignature(const std::vector<uint8_t>& data) {
    if (!initialized) {
        throw SshException("Private key not loaded.");
    }

    std::vector<uint8_t> hash = SHA256::hash(data);

    std::vector<uint8_t> digestInfo = createDigestInfo(hash);

    size_t k = (mpz_sizeinbase(n.get_mpz_t(), 2) + 7) / 8;
    std::vector<uint8_t> em = applyPKCS1Padding(digestInfo, k);

    mpz_class m = bytesToMpz(em);
    mpz_class s;
    mpz_powm(s.get_mpz_t(), m.get_mpz_t(), d.get_mpz_t(), n.get_mpz_t());

    std::vector<uint8_t> sig = mpzToBytes(s, k);

    std::vector<uint8_t> sshSig;
    SshTypeHelper::appendAsSshStringToVector(sshSig, "rsa-sha2-256");
    SshTypeHelper::appendAsSshStringToVector(sshSig, std::string(sig.begin(), sig.end()));
    
    return sshSig;
}

std::vector<uint8_t> RSA::base64Decode(const std::string& input) {
    static const std::string base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<uint8_t> result;
    int val = 0, valb = -8;

    for (char c : input) {
        if (c == ' ' || c == '\n' || c == '\r') continue;
        if (c == '=') break;

        size_t pos = base64_chars.find(c);
        if (pos == std::string::npos) {
            throw SshException("Invalid Base64 character");
        }

        val = (val << 6) + static_cast<int>(pos);
        valb += 6;
        if (valb >= 0) {
            result.push_back((val >> valb) & 0xFF);
            valb -= 8;
        }
    }
    return result;
}

void RSA::parseDERPrivateKey(const std::vector<uint8_t>& der) {
    size_t index = 0;
    if (der[index++] != 0x30) {
        throw SshException("Invalid DER format");
    }

    if ((der[index] & 0x80) != 0) {
        index += (der[index] & 0x7F) + 1;
    } else {
        index++;
    }

    mpz_class version;
    index = extractDerInteger(der, index, version);

    index = extractDerInteger(der, index, n);
    index = extractDerInteger(der, index, e);
    index = extractDerInteger(der, index, d);
    index = extractDerInteger(der, index, p);
    index = extractDerInteger(der, index, q);
}

size_t RSA::extractDerInteger(const std::vector<uint8_t>& der, size_t index, mpz_class& value) {
    if (der[index++] != 0x02) {
        throw SshException("Expected INTEGER tag");
    }

    size_t length = der[index++];
    if (length & 0x80) {
        size_t lenBytes = length & 0x7F;
        length = 0;
        for (size_t i = 0; i < lenBytes; i++) {
            length = (length << 8) | der[index++];
        }
    }

    std::vector<uint8_t> intBytes(der.begin() + index, der.begin() + index + length);
    mpz_import(value.get_mpz_t(), intBytes.size(), 1, 1, 0, 0, intBytes.data());
    return index + length;
}

std::vector<uint8_t> RSA::createDigestInfo(const std::vector<uint8_t>& hash) {
    std::vector<uint8_t> digestInfo;
    digestInfo.push_back(0x30);
    digestInfo.push_back(0x31);
    digestInfo.push_back(0x30);
    digestInfo.push_back(0x0d);
    digestInfo.push_back(0x06);
    digestInfo.push_back(0x09);
    digestInfo.insert(digestInfo.end(), {0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01});
    digestInfo.push_back(0x05);
    digestInfo.push_back(0x00);
    digestInfo.push_back(0x04);
    digestInfo.push_back(0x20);
    digestInfo.insert(digestInfo.end(), hash.begin(), hash.end());
    return digestInfo;
}

std::vector<uint8_t> RSA::applyPKCS1Padding(const std::vector<uint8_t>& t, size_t k) {
    if (t.size() + 11 > k) {
        throw SshException("Key too small for PKCS#1 padding");
    }

    std::vector<uint8_t> em(k);
    em[0] = 0x00;
    em[1] = 0x01;
    size_t psLen = k - t.size() - 3;
    std::fill(em.begin() + 2, em.begin() + 2 + psLen, 0xFF);
    em[2 + psLen] = 0x00;
    std::copy(t.begin(), t.end(), em.begin() + 2 + psLen + 1);
    return em;
}

std::vector<uint8_t> RSA::mpzToBytes(const mpz_class& value, size_t desired_length) {
    size_t count = (mpz_sizeinbase(value.get_mpz_t(), 2) + 7) / 8;
    if (desired_length == 0) desired_length = count;
    std::vector<uint8_t> result(desired_length, 0);
    size_t offset = desired_length - count;
    mpz_export(result.data() + offset, nullptr, 1, 1, 0, 0, value.get_mpz_t());
    return result;
}

mpz_class RSA::bytesToMpz(const std::vector<uint8_t>& bytes) {
    mpz_class result;
    mpz_import(result.get_mpz_t(), bytes.size(), 1, 1, 0, 0, bytes.data());
    return result;
}