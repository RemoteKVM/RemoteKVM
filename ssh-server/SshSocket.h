#pragma once

#include <vector>
#include <cstdint>
#include <unistd.h>
#include <errno.h>
#include <gmpxx.h>
#include "SshPacket.h"
#include "SshTypeHelper.h"
#include "SshException.h"
#include "AESCTR.h"
#include "HMAC.h"
#include "SSHKeyDerivation.h"

#define SSH_MSG_DISCONNECT 1
#define SSH_MSG_IGNORE 2
#define SSH_DISCONNECT_BY_APPLICATION 11 // reason code


class SshSocket {
public:
    explicit SshSocket(int fd);
    ~SshSocket();

    std::vector<uint8_t> readPacket();
    void sendPacket(const std::vector<uint8_t>& payload);

    void setSheredSecret(const mpz_class& secret) {
        sharedSecret = secret;
    }

    void setSessionID(const std::vector<uint8_t>& id) {
        sessionID = id;
    }

    void startEncryptedPhase();

    void sendDisconnectMessage();

private:
    int sockfd;
    
    mpz_class sharedSecret;
    std::vector<uint8_t> sessionID;

    AESCTR aesRecv;
    AESCTR aesSend;
    SSHHMAC hmacRecv;
    SSHHMAC hmacSend;

    bool isEncryptedPhase = false;

    // helper functions:
    void readBytes(void* buf, size_t count);
    void writeAll(const void* buf, size_t count);

    std::vector<uint8_t> readPacketEncrypted();
    std::vector<uint8_t> readPacketUnencrypted();

    void sendPacketEncrypted(const std::vector<uint8_t>& payload);
    void sendPacketUnencrypted(const std::vector<uint8_t>& payload);

    void hendleSpecialMessages(std::vector<uint8_t>& payload);
};