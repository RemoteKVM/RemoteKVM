#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <random>
#include "SshSocket.h"
#include "SshPacket.h"

#define SSH_MSG_KEX_INIT 20

static bool checkIfInList(const std::vector<std::string>& list, const std::string& value);

class SshMsgKexInit {
public:
    std::vector<uint8_t> cookie; // Must be exactly 16 bytes
    std::vector<std::string> kex_algorithms;
    std::vector<std::string> server_host_key_algorithms;
    std::vector<std::string> encryption_algorithms_client_to_server;
    std::vector<std::string> encryption_algorithms_server_to_client;
    std::vector<std::string> mac_algorithms_client_to_server;
    std::vector<std::string> mac_algorithms_server_to_client;
    std::vector<std::string> compression_algorithms_client_to_server;
    std::vector<std::string> compression_algorithms_server_to_client;
    std::vector<std::string> languages_client_to_server;
    std::vector<std::string> languages_server_to_client;
    bool first_kex_packet_follows;

    SshMsgKexInit();
    std::vector<uint8_t> serialize() const;
    static SshMsgKexInit deserialize(const std::vector<uint8_t>& payload);
    bool checkSportedAlgoritems(const SshMsgKexInit& corrctAlgoritems);

private:
    static bool checkIfInList(const std::vector<std::string>& list, const std::string& value);
};