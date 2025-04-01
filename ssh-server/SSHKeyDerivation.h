#pragma once

#include <vector>
#include <gmpxx.h>
#include "SHA256.h"

// Modified SSHKeyDerivation to use our SHA-256 implementation
class SSHKeyDerivation {
private:
    std::vector<uint8_t> K;    // The shared secret
    std::vector<uint8_t> H;    // The exchange hash
    std::vector<uint8_t> sessionId; // The session ID (first H)
    
    // Compute a key with a specific letter
    std::vector<uint8_t> computeKey(char letter, size_t keyLength) {
        // K || H || letter || session_id
        std::vector<uint8_t> data;
        data.insert(data.end(), K.begin(), K.end());
        //SshTypeHelper::appendAsMpintToVector(data, K);
        data.insert(data.end(), H.begin(), H.end());
        data.push_back(letter);
        data.insert(data.end(), sessionId.begin(), sessionId.end());
        
        std::vector<uint8_t> result = SHA256::hash(data);
        
        // If we need more key material, we compute K || H || result
        while (result.size() < keyLength) {
            std::vector<uint8_t> extendData;
            extendData.insert(extendData.end(), K.begin(), K.end());
            extendData.insert(extendData.end(), H.begin(), H.end());
            extendData.insert(extendData.end(), result.begin(), result.end());
            
            std::vector<uint8_t> additional = SHA256::hash(extendData);
            result.insert(result.end(), additional.begin(), additional.end());
        }
        
        // Truncate if necessary
        if (result.size() > keyLength) {
            result.resize(keyLength);
        }
        
        return result;
    }
    
public:
    // Initialize with K and H from the key exchange
    SSHKeyDerivation(const std::vector<uint8_t>& sharedSecret, const std::vector<uint8_t>& exchangeHash) 
        : K(sharedSecret), H(exchangeHash), sessionId(exchangeHash) {}
    
    // If session ID is already established (rekey)
    SSHKeyDerivation(const std::vector<uint8_t>& sharedSecret, 
                     const std::vector<uint8_t>& exchangeHash,
                     const std::vector<uint8_t>& existingSessionId) 
        : K(sharedSecret), H(exchangeHash), sessionId(existingSessionId) {}
    
    // Alternative constructor with MPZ for shared secret
    SSHKeyDerivation(const mpz_class& sharedSecret, const std::vector<uint8_t>& exchangeHash) 
        : H(exchangeHash), sessionId(exchangeHash) {
        
        std::string sharedSecretStr = SshTypeHelper::mpzToMpint(sharedSecret);
        K.assign(sharedSecretStr.begin(), sharedSecretStr.end());
    }
    
    // Get Initial IV client to server: HASH(K || H || "A" || session_id)
    std::vector<uint8_t> getIVClientToServer(size_t length = 16) {
        return computeKey('A', length);
    }
    
    // Get Initial IV server to client: HASH(K || H || "B" || session_id)
    std::vector<uint8_t> getIVServerToClient(size_t length = 16) {
        return computeKey('B', length);
    }
    
    // Get Encryption key client to server: HASH(K || H || "C" || session_id)
    std::vector<uint8_t> getEncryptionKeyClientToServer(size_t length = 32) {
        return computeKey('C', length);
    }
    
    // Get Encryption key server to client: HASH(K || H || "D" || session_id)
    std::vector<uint8_t> getEncryptionKeyServerToClient(size_t length = 32) {
        return computeKey('D', length);
    }
    
    // Get MAC key client to server: HASH(K || H || "E" || session_id)
    std::vector<uint8_t> getMACKeyClientToServer(size_t length = 32) {
        return computeKey('E', length);
    }
    
    // Get MAC key server to client: HASH(K || H || "F" || session_id)
    std::vector<uint8_t> getMACKeyServerToClient(size_t length = 32) {
        return computeKey('F', length);
    }
    
    // Helper function to convert a vector to a hex string
    static std::string toHexString(const std::vector<uint8_t>& data) {
        std::string result;
        char hex[3];
        
        for (uint8_t byte : data) {
            sprintf(hex, "%02x", byte);
            result += hex;
        }
        
        return result;
    }
};