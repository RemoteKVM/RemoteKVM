#include "SshMsgKexInit.h"
#include "SshTypeHelper.h"


SshMsgKexInit::SshMsgKexInit() : cookie(16), first_kex_packet_follows(false) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    for (size_t i = 0; i < 16; ++i) {
        cookie[i] = static_cast<uint8_t>(dis(gen));
    }
}

std::vector<uint8_t> SshMsgKexInit::serialize() const {
    std::vector<uint8_t> payload;
    payload.push_back(SSH_MSG_KEX_INIT);
    if (cookie.size() != 16) {
        throw std::runtime_error("Cookie must be 16 bytes");
    }
    payload.insert(payload.end(), cookie.begin(), cookie.end());
    SshTypeHelper::appendNameListToVector(payload, kex_algorithms);
    SshTypeHelper::appendNameListToVector(payload, server_host_key_algorithms);
    SshTypeHelper::appendNameListToVector(payload, encryption_algorithms_client_to_server);
    SshTypeHelper::appendNameListToVector(payload, encryption_algorithms_server_to_client);
    SshTypeHelper::appendNameListToVector(payload, mac_algorithms_client_to_server);
    SshTypeHelper::appendNameListToVector(payload, mac_algorithms_server_to_client);
    SshTypeHelper::appendNameListToVector(payload, compression_algorithms_client_to_server);
    SshTypeHelper::appendNameListToVector(payload, compression_algorithms_server_to_client);
    SshTypeHelper::appendNameListToVector(payload, languages_client_to_server);
    SshTypeHelper::appendNameListToVector(payload, languages_server_to_client);
    payload.push_back(first_kex_packet_follows ? 1 : 0);
    payload.push_back(0);
    payload.push_back(0);
    payload.push_back(0);
    payload.push_back(0);
    return payload;
}

SshMsgKexInit SshMsgKexInit::deserialize(const std::vector<uint8_t>& payload) {
    SshMsgKexInit msg;
    size_t offset = 0;
    if (offset >= payload.size()) {
        throw std::runtime_error("Payload too short for SSH_MSG_KEX_INIT");
    }
    uint8_t msgType = payload[offset++];
    if (msgType != 0x14) {
        throw std::runtime_error("Not an SSH_MSG_KEX_INIT message");
    }
    if (offset + 16 > payload.size()) {
        throw std::runtime_error("Payload too short for cookie");
    }
    msg.cookie.assign(payload.begin() + offset, payload.begin() + offset + 16);
    offset += 16;
    msg.kex_algorithms = SshTypeHelper::readSshStringsList(payload, offset);
    msg.server_host_key_algorithms = SshTypeHelper::readSshStringsList(payload, offset);
    msg.encryption_algorithms_client_to_server = SshTypeHelper::readSshStringsList(payload, offset);
    msg.encryption_algorithms_server_to_client = SshTypeHelper::readSshStringsList(payload, offset);
    msg.mac_algorithms_client_to_server = SshTypeHelper::readSshStringsList(payload, offset);
    msg.mac_algorithms_server_to_client = SshTypeHelper::readSshStringsList(payload, offset);
    msg.compression_algorithms_client_to_server = SshTypeHelper::readSshStringsList(payload, offset);
    msg.compression_algorithms_server_to_client = SshTypeHelper::readSshStringsList(payload, offset);
    msg.languages_client_to_server = SshTypeHelper::readSshStringsList(payload, offset);
    msg.languages_server_to_client = SshTypeHelper::readSshStringsList(payload, offset);
    if (offset >= payload.size()) {
        throw std::runtime_error("Payload too short for first_kex_packet_follows");
    }
    msg.first_kex_packet_follows = (payload[offset++] != 0);
    if (offset + 4 > payload.size()) {
        throw std::runtime_error("Payload too short for reserved field");
    }
    uint32_t netReserved;
    memcpy(&netReserved, payload.data() + offset, 4);
    uint32_t reserved = SshTypeHelper::swapByteOrder(netReserved);
    offset += 4;
    if (reserved != 0) {
        throw std::runtime_error("Reserved field is non-zero");
    }
    return msg;
}

bool SshMsgKexInit::checkSportedAlgoritems(const SshMsgKexInit& corrctAlgoritems) {
    return (checkIfInList(this->kex_algorithms, corrctAlgoritems.kex_algorithms[0]) &&
            checkIfInList(this->server_host_key_algorithms, corrctAlgoritems.server_host_key_algorithms[0]) &&
            checkIfInList(this->encryption_algorithms_client_to_server, corrctAlgoritems.encryption_algorithms_client_to_server[0]) &&
            checkIfInList(this->encryption_algorithms_server_to_client, corrctAlgoritems.encryption_algorithms_server_to_client[0]) &&
            checkIfInList(this->mac_algorithms_client_to_server, corrctAlgoritems.mac_algorithms_client_to_server[0]) &&
            checkIfInList(this->mac_algorithms_server_to_client, corrctAlgoritems.mac_algorithms_server_to_client[0]) &&
            checkIfInList(this->compression_algorithms_client_to_server, corrctAlgoritems.compression_algorithms_client_to_server[0]) &&
            checkIfInList(this->compression_algorithms_server_to_client, corrctAlgoritems.compression_algorithms_server_to_client[0]));
}

bool SshMsgKexInit::checkIfInList(const std::vector<std::string>& list, const std::string& value)
{
    for (const auto& item : list)
    {
        if (item == value)
        {
            return true;
        }
    }
    return false;
}
