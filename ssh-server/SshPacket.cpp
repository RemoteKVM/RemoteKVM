#include "SshPacket.h"

// Serialize the packet into a vector of bytes following the SSH binary packet format.
std::vector<uint8_t> SshPacket::serialize(std::vector<uint8_t> payload) {
    // For unencrypted packets, the block size is defined as 8.
    const uint32_t block_size = 16;
    uint32_t payload_size = payload.size();

    // Calculate the minimum padding length: at least 4 bytes and such that
    // (payload_size + 1 (for padding_length) + padding_length) is a multiple of block_size.
    uint32_t pad_length = block_size - ((payload_size + 5) % block_size);
    if (pad_length < 4) {
        pad_length += block_size;
    }
    
    // The packet_length field (4 bytes) is the length of the remainder of the packet:
    // payload + padding_length (1 byte) + padding.
    uint32_t packet_length = static_cast<uint32_t>(payload_size + pad_length + 1);

    // Allocate a vector to hold the full packet:
    // 4 bytes for packet_length + packet_length bytes.
    std::vector<uint8_t> packet(4 + packet_length);

    // Write packet_length in network byte order.
    uint32_t net_packet_length = SshTypeHelper::swapByteOrder(packet_length);
    memcpy(packet.data(), &net_packet_length, sizeof(net_packet_length));

    // Write the padding length.
    packet[4] = static_cast<uint8_t>(pad_length);

    // Copy the payload bytes.
    if (payload_size > 0) {
        memcpy(packet.data() + 5, payload.data(), payload_size);
    }

    // Generate random padding bytes.
    std::vector<uint8_t> padding(pad_length);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < pad_length; ++i) {
        padding[i] = static_cast<uint8_t>(dis(gen));
    }

    // Append the padding bytes.
    memcpy(packet.data() + 5 + payload_size, padding.data(), pad_length);

    return packet;
}