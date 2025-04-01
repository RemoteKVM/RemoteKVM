#include "SshSocket.h"

SshSocket::SshSocket(int fd) : sockfd(fd) {}

SshSocket::~SshSocket() {}

void SshSocket::readBytes(void* buf, size_t count) {
    uint8_t* buffer = reinterpret_cast<uint8_t*>(buf);
    size_t totalRead = 0;
    while (totalRead < count) {
        ssize_t n = ::read(sockfd, buffer + totalRead, count - totalRead);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw SshException("Error reading from socket");
        } else if (n == 0) {
            throw SshException("Connection closed unexpectedly");
        }
        totalRead += n;
    }
}

void SshSocket::writeAll(const void* buf, size_t count) {
    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(buf);
    size_t totalWritten = 0;
    while (totalWritten < count) {
        ssize_t n = ::write(sockfd, buffer + totalWritten, count - totalWritten);
        if (n < 0) {
            if (errno == EINTR) continue;
            throw SshException("Error writing to socket");
        }
        totalWritten += n;
    }
}

std::vector<uint8_t> SshSocket::readPacketUnencrypted() {
    uint32_t net_packet_length; // packet length in network byte order
    readBytes(&net_packet_length, sizeof(net_packet_length));
    uint32_t packet_length = SshTypeHelper::swapByteOrder(net_packet_length);

    uint8_t pad_length;
    readBytes(&pad_length, sizeof(pad_length));

    std::vector<uint8_t> payload(packet_length - 1 - pad_length);
    readBytes(payload.data(), packet_length - 1 - pad_length);

    std::vector<uint8_t> padding(pad_length);
    readBytes(padding.data(), pad_length);

    hmacRecv.increase_sequence_number();

    hendleSpecialMessages(payload);

    return payload;
}

std::vector<uint8_t> SshSocket::readPacketEncrypted() {
    // Read the encrypted packet length
    uint32_t net_packet_length_enc; // network byte order
    readBytes(&net_packet_length_enc, sizeof(net_packet_length_enc));
    
    // Create a copy of the current counter state before any decryption
    mpz_class originalCounter = aesRecv.getCounter();
    
    // Decrypt the packet length
    uint8_t net_packet_length_dec_arr[4];
    aesRecv.process((uint8_t*)&net_packet_length_enc, net_packet_length_dec_arr, 4);
    
    uint32_t packet_length = SshTypeHelper::swapByteOrder(*(uint32_t*)net_packet_length_dec_arr);

    aesRecv.setCounter(originalCounter); // Restore the counter state

    std::vector<uint8_t> full_packet(4 + packet_length);
    memcpy(full_packet.data(), &net_packet_length_enc, sizeof(net_packet_length_enc));
    readBytes(full_packet.data() + 4, packet_length);
    
    // Decrypt the payload and padding together
    std::vector<uint8_t> decrypted_packet(packet_length + sizeof(net_packet_length_enc));
    aesRecv.process(full_packet.data(), decrypted_packet.data(), packet_length + sizeof(net_packet_length_enc));
    
    // Read the padding length
    uint8_t pad_length = decrypted_packet[sizeof(net_packet_length_enc)];
    
    std::vector<uint8_t> payload(decrypted_packet.begin() + sizeof(net_packet_length_enc) + sizeof(pad_length), decrypted_packet.end() - pad_length);
    
    // read the MAC
    std::vector<uint8_t> mac(32);
    readBytes(mac.data(), 32);
    
    if (!hmacRecv.verify_hmac(decrypted_packet, mac)) {
        throw SshException("MAC verification failed");
    }

    hendleSpecialMessages(payload);

    //return payload;
    return payload;
}

void SshSocket::hendleSpecialMessages(std::vector<uint8_t>& payload) {
    if (payload[0] == SSH_MSG_DISCONNECT) {
        throw SshException("Client disconnected");
    } else if (payload[0] == SSH_MSG_IGNORE) {
        payload = readPacket(); // ignore the message and read the next one
    }
}

std::vector<uint8_t> SshSocket::readPacket() {
    if (isEncryptedPhase) {
        return readPacketEncrypted(); // after key exchange, packets are encrypted
    } else {
        return readPacketUnencrypted(); // before key exchange, packets are unencrypted
    }
}

void SshSocket::sendPacketUnencrypted(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> serialized = SshPacket::serialize(payload);
    writeAll(serialized.data(), serialized.size());
    hmacSend.increase_sequence_number();
}

void SshSocket::sendPacketEncrypted(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> serialized_dec = SshPacket::serialize(payload); // the serialized payload (packet length + padding length + payload + padding)

    std::vector<uint8_t> serialized_enc(serialized_dec.size());
    aesSend.process(serialized_dec.data(), serialized_enc.data(), serialized_dec.size());

    // Calculate the MAC
    std::vector<uint8_t> mac = hmacSend.generate_hmac(serialized_dec);
    
    serialized_enc.insert(serialized_enc.end(), mac.begin(), mac.end());
    writeAll(serialized_enc.data(), serialized_enc.size());
}

void SshSocket::sendPacket(const std::vector<uint8_t>& payload) {
    if (isEncryptedPhase) {
        sendPacketEncrypted(payload);
    } else {
        sendPacketUnencrypted(payload);
    }
}

void SshSocket::startEncryptedPhase() {
    isEncryptedPhase = true;
    SSHKeyDerivation sshKeyDerivation(sharedSecret, sessionID);

    std::vector<uint8_t> iv_client_to_server = sshKeyDerivation.getIVClientToServer();
    std::vector<uint8_t> key_client_to_server = sshKeyDerivation.getEncryptionKeyClientToServer();
    aesRecv.setKeyAndIV(key_client_to_server.data(), iv_client_to_server.data());

    std::vector<uint8_t> iv_server_to_client = sshKeyDerivation.getIVServerToClient();
    std::vector<uint8_t> key_server_to_client = sshKeyDerivation.getEncryptionKeyServerToClient();
    aesSend.setKeyAndIV(key_server_to_client.data(), iv_server_to_client.data());

    std::vector<uint8_t> mac_key_client_to_server = sshKeyDerivation.getMACKeyClientToServer();
    hmacRecv.set_mac_key(mac_key_client_to_server);

    std::vector<uint8_t> mac_key_server_to_client = sshKeyDerivation.getMACKeyServerToClient();
    hmacSend.set_mac_key(mac_key_server_to_client);
}

void SshSocket::sendDisconnectMessage() {
    std::vector<uint8_t> payload;
    payload.push_back(SSH_MSG_DISCONNECT);
    SshTypeHelper::writeUint32(payload, SSH_DISCONNECT_BY_APPLICATION); // reason code
    SshTypeHelper::appendAsSshStringToVector(payload, "See you soon! Check out our website at https://remotekvm.online. Yours, Liron & Omer");
    sendPacket(payload);
}