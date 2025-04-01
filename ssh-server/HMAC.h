#include <vector>
#include <cstdint>
#include <cstring> // For memcpy
#include <stdexcept>  // For std::runtime_error
#include <iostream>   // For debugging (remove in production)
#include "SHA256.h"

class SSHHMAC {
public:
    SSHHMAC() : sequence_number_(0) {
    }

    void set_mac_key(const std::vector<uint8_t>& mac_key) {
        mac_key_ = mac_key;
    }

    // Verify HMAC received from client
    bool verify_hmac(const std::vector<uint8_t>& received_packet, const std::vector<uint8_t>& received_hmac) {
        std::vector<uint8_t> expected_hmac = calculate_hmac(received_packet);

        if (expected_hmac.size() != received_hmac.size()) {
            std::cerr << "HMAC size mismatch: expected " << expected_hmac.size() << ", received " << received_hmac.size() << std::endl;
            return false;
        }

        // Constant-time comparison to prevent timing attacks
        for (size_t i = 0; i < expected_hmac.size(); ++i) {
            if (expected_hmac[i] != received_hmac[i]) {
                return false;
            }
        }
        return true;
    }

    // Generate HMAC for server's outgoing packet
    std::vector<uint8_t> generate_hmac(const std::vector<uint8_t>& packet) {
        return calculate_hmac(packet);
    }

    void increase_sequence_number() {
        sequence_number_++;
    }


private:
    std::vector<uint8_t> calculate_hmac(const std::vector<uint8_t>& packet) {
        // Prepare the data for HMAC calculation: sequence number + packet data
        std::vector<uint8_t> data_to_hash;

        // Append the sequence number (network byte order - Big Endian)
        data_to_hash.push_back((sequence_number_ >> 24) & 0xFF);
        data_to_hash.push_back((sequence_number_ >> 16) & 0xFF);
        data_to_hash.push_back((sequence_number_ >> 8) & 0xFF);
        data_to_hash.push_back(sequence_number_ & 0xFF);

        // Append the packet data
        data_to_hash.insert(data_to_hash.end(), packet.begin(), packet.end());

        // Calculate HMAC-SHA256

        std::vector<uint8_t> hmac = HMAC_SHA256(mac_key_, data_to_hash); // Use the new HMAC_SHA256 function


        // Increment sequence number
        sequence_number_++;

        return hmac;
    }



    std::vector<uint8_t> HMAC_SHA256(const std::vector<uint8_t>& key, const std::vector<uint8_t>& data) {
        size_t key_len = key.size();
        size_t data_len = data.size();

        std::array<uint8_t, 64> ipad, opad, k_ipad, k_opad;
        ipad.fill(0x36);
        opad.fill(0x5c);
        k_ipad.fill(0);
        k_opad.fill(0);

        std::vector<uint8_t> temp_key;
        if (key_len > 64) {
            temp_key = SHA256::hash(key);
            key_len = temp_key.size();
        }

        // Copy key into padded buffers
        if (key_len <= 64) {
            std::memcpy(k_ipad.data(), key.data(), key_len);
            std::memcpy(k_opad.data(), key.data(), key_len);
        } else {
            std::memcpy(k_ipad.data(), temp_key.data(), temp_key.size());
            std::memcpy(k_opad.data(), temp_key.data(), temp_key.size());
        }


        // XOR with ipad and opad
        for (int i = 0; i < 64; ++i) {
            k_ipad[i] ^= ipad[i];
            k_opad[i] ^= opad[i];
        }


        // Inner hash: SHA256(K XOR ipad || data)
        std::vector<uint8_t> inner_data(k_ipad.begin(), k_ipad.end());
        inner_data.insert(inner_data.end(), data.begin(), data.end());
        std::vector<uint8_t> inner_hash = SHA256::hash(inner_data);

        // Outer hash: SHA256(K XOR opad || inner_hash)
        std::vector<uint8_t> outer_data(k_opad.begin(), k_opad.end());
        outer_data.insert(outer_data.end(), inner_hash.begin(), inner_hash.end());
        std::vector<uint8_t> hmac = SHA256::hash(outer_data);

        return hmac;
    }


private:
    std::vector<uint8_t> mac_key_;
    uint32_t sequence_number_;
};