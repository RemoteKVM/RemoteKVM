#pragma once

#include <vector>
#include "SshSocket.h"
#include "SshTypeHelper.h"
#include "SshException.h"


#define SSH_MSG_CHANNEL_OPEN 90
#define SSH_MSG_CHANNEL_OPEN_CONFIRMATION 91
#define SSH_MSG_CHANNEL_WINDOW_ADJUST 93
#define SSH_MSG_CHANNEL_DATA 94
#define SSH_MSG_CHANNEL_REQUEST 98
#define SSH_MSG_CHANNEL_SUCCESS 99

#define SESSION_CHANNEL_TYPE "session"
#define SHELL_REQUEST_TYPE "shell"

class ChannelsHandler {
public:
    ChannelsHandler(SshSocket* sshSocket) : sshSocket(sshSocket) {}
    ~ChannelsHandler() {}

    void InitChannels() {
        resiveOpenChannelMsg();
        sendOpenChannelConfirmation();
        resvAndAnswerChannelRequest();
    }

    void sendDataOnChannel(std::string data) {
        std::vector<uint8_t> packet;
        packet.push_back(SSH_MSG_CHANNEL_DATA);
        SshTypeHelper::writeUint32(packet, senderChannelId); // sender channel
        SshTypeHelper::appendAsSshStringToVector(packet, data);
        sshSocket->sendPacket(packet);
    }

    std::string readDataFromChannel() {
        std::vector<uint8_t> msg = sshSocket->readPacket();

        if (msg[0] == SSH_MSG_CHANNEL_WINDOW_ADJUST) {
            return ""; // ignore window adjust
        }

        size_t offset = 1 + 4; // offset is the offset to the start of the string
        std::string data = SshTypeHelper::readSshString(msg, offset);

        if (data == "window-change") {
            return ""; // ignore window adjust
        }
        if (msg[0] == SSH_MSG_CHANNEL_REQUEST) {
            replayChannelRequest(); // replay with success
            return "";
        }
        else if (msg[0] != SSH_MSG_CHANNEL_DATA) {
            throw SshException("Expected SSH_MSG_CHANNEL_DATA message from client! + got: " + std::to_string(msg[0]));
        }

        return data;
    }

private:
    SshSocket* sshSocket;

    uint32_t senderChannelId;
    uint32_t recipientChannelId;
    uint32_t initialWindowSize;
    uint32_t maximumPacketSize;

    void resiveOpenChannelMsg() {
        std::vector<uint8_t> serviceRequest = sshSocket->readPacket();

        if (serviceRequest[0] != SSH_MSG_CHANNEL_OPEN) {
            throw SshException("Expected SSH_MSG_CHANNEL_OPEN message from client!");
        }

        size_t offset = 1; // offset is the offset to the start of the string
        std::string channelSize = SshTypeHelper::readSshString(serviceRequest, offset); 

        if (channelSize != SESSION_CHANNEL_TYPE) {
            throw SshException("Expected channel type 'session'!");
        }

        senderChannelId = SshTypeHelper::readUint32(serviceRequest, offset);
        initialWindowSize = SshTypeHelper::readUint32(serviceRequest, offset);
        maximumPacketSize = SshTypeHelper::readUint32(serviceRequest, offset);

        recipientChannelId = 0;
    }

    void sendOpenChannelConfirmation() {
        std::vector<uint8_t> packet;
        packet.push_back(SSH_MSG_CHANNEL_OPEN_CONFIRMATION);
        SshTypeHelper::writeUint32(packet, senderChannelId); // sender channel
        SshTypeHelper::writeUint32(packet, recipientChannelId); // recipient channel
        SshTypeHelper::writeUint32(packet, initialWindowSize); // initial window size
        SshTypeHelper::writeUint32(packet, maximumPacketSize); // maximum packet size
        sshSocket->sendPacket(packet);
    }

    // We don't support pty so we just answer with success.
    void resvAndAnswerChannelRequest() {
        while (true) { // run until we get a shell request (the client wants to start a shell even after we answer with success about the pty. different clients sends different number of requests before the shell request)
            std::vector<uint8_t> msg = sshSocket->readPacket();
                    
            if (msg[0] != SSH_MSG_CHANNEL_REQUEST) {
                throw SshException("Expected SSH_MSG_CHANNEL_REQUEST message from client!");
            }

            replayChannelRequest();

            size_t offset = 1 + sizeof(uint32_t); // offset is the offset to the start of the string
            std::string requestType = SshTypeHelper::readSshString(msg, offset);
            if (requestType == SHELL_REQUEST_TYPE) {
                break; 
            }
        }
        
    }

    void replayChannelRequest() {
        std::vector<uint8_t> response;
        response.push_back(SSH_MSG_CHANNEL_SUCCESS);
        SshTypeHelper::writeUint32(response, senderChannelId); // sender channel
        sshSocket->sendPacket(response);
    }
};