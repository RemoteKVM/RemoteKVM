#pragma once

#include <vector>
#include "SshSocket.h"
#include "SshTypeHelper.h"
#include "SshException.h"
#include "DbHandler.h"
#include "VmsHandler.h"

#define SSH_MSG_SERVICE_REQUEST 5
#define SSH_MSG_SERVICE_ACCEPT 6

#define SSH_MSG_USERAUTH_REQUEST 50
#define SSH_MSG_USERAUTH_FAILURE 51
#define SSH_MSG_USERAUTH_SUCCESS 52

#define SSH_MSG_USERAUTH_STRING "ssh-userauth"

#define USERNAME_SEPARATOR '#'

enum UserVmSelectionMethod {
    VM_NOT_PRE_SELECTED = 0,
    VM_PRE_SELECTED_WITHOUT_TOKEN = 1,
    VM_PRE_SELECTED_WITH_TOKEN = 2
};

class AuthenticationHandler {
public:
    AuthenticationHandler(SshSocket* sshSocket, VmsHandler* vmsHandler, DbHandler* dbHandler) :  sshSocket(sshSocket), vmsHandler(vmsHandler), dbHandler(dbHandler) {}
    ~AuthenticationHandler() {}

    void perform_authentication() {
        resiveAuthServiceRequest(); // read the service request from the client (SSH_MSG_SERVICE_REQUEST)
        sendAuthServiceRequest(); // replay with a service request accepted message (SSH_MSG_SERVICE_ACCEPT)

        std::string usernameInput;
        std::string password;

        std::string vmNameOrId;
        std::string token;

        ApiResponse response;

        while (true) {
            if (!readUserAuthRequest(usernameInput, password)) {
                continue; // the client requested a different authentication method, try again
            }

            switch (recognizeUserVmSelectionMethod(usernameInput))
            {
            case VM_NOT_PRE_SELECTED:
                response = dbHandler->validateUser(usernameInput, password);
                if (response.success) {
                    sendSuccessMessage();
                    vmsHandler->setVms(response.vms);
                    return;
                } else {
                    sendFailureMessage();
                }
                break;
            case VM_PRE_SELECTED_WITHOUT_TOKEN:
                splitUsernameWithoutToken(usernameInput, vmNameOrId);
                response = dbHandler->validateVm(usernameInput, password, vmNameOrId);
                if (response.success) {
                    sendSuccessMessage();
                    vmsHandler->setSelectedVm(response.vm);
                    return;
                } else {
                    sendFailureMessage();
                }
                break;
            case VM_PRE_SELECTED_WITH_TOKEN:
                splitUsernameWithToken(usernameInput, vmNameOrId, token);
                response = dbHandler->validateTerminalToken(usernameInput, std::stoi(vmNameOrId), token);
                if (response.success) {
                    sendSuccessMessage();
                    vmsHandler->setSelectedVm(response.vm);
                    return;
                } else {
                    sendFailureMessage();
                }
                break;
            default:
                break;
            }
            
        }
    }

private:
    SshSocket* sshSocket;
    VmsHandler* vmsHandler;
    DbHandler* dbHandler;

    void resiveAuthServiceRequest() {
        std::vector<uint8_t> serviceRequest = sshSocket->readPacket();

        if (serviceRequest[0] != SSH_MSG_SERVICE_REQUEST) {
            throw SshException("Expected SSH_MSG_SERVICE_REQUEST message from client!");
        }

        size_t offset = 1; // offset is the offset to the start of the string
        std::string serviceName = SshTypeHelper::readSshString(serviceRequest, offset); 

        if (serviceName != SSH_MSG_USERAUTH_STRING) {
            throw SshException("Expected service name 'ssh-userauth'!");
        }
    }

    void sendAuthServiceRequest() {
        std::vector<uint8_t> serviceRequest;
        serviceRequest.push_back(SSH_MSG_SERVICE_ACCEPT);
        SshTypeHelper::appendAsSshStringToVector(serviceRequest, SSH_MSG_USERAUTH_STRING);
        sshSocket->sendPacket(serviceRequest);
    }

    void sendFailureMessage() {
        std::vector<uint8_t> failureMsg;
        failureMsg.push_back(SSH_MSG_USERAUTH_FAILURE);
        SshTypeHelper::appendAsSshStringToVector(failureMsg, "password");
        failureMsg.push_back(0); // partial success
        sshSocket->sendPacket(failureMsg);
    }

    void sendSuccessMessage() {
        std::vector<uint8_t> successMsg;
        successMsg.push_back(SSH_MSG_USERAUTH_SUCCESS);
        sshSocket->sendPacket(successMsg);
    }

    bool readUserAuthRequest(std::string& username, std::string& password) {
        std::vector<uint8_t> userAuthRequest = sshSocket->readPacket();

        if (userAuthRequest[0] != SSH_MSG_USERAUTH_REQUEST) {
            throw SshException("Expected SSH_MSG_USERAUTH_REQUEST message from client!");
        }

        size_t offset = 1; // offset is the offset to the start of the string
        username = SshTypeHelper::readSshString(userAuthRequest, offset); 
        std::string serviceName = SshTypeHelper::readSshString(userAuthRequest, offset); 
        std::string methodName = SshTypeHelper::readSshString(userAuthRequest, offset); 

        if (methodName != "password") {
            sendFailureMessage();
            return false;
        }

        offset++; // skip the boolean value (allways 0)
        password = SshTypeHelper::readSshString(userAuthRequest, offset);

        return true;
    }

    UserVmSelectionMethod recognizeUserVmSelectionMethod(std::string& username) {
        return UserVmSelectionMethod(std::count(username.begin(), username.end(), USERNAME_SEPARATOR));
    }

    void splitUsernameWithoutToken(std::string& username, std::string& vmName) {
        size_t pos = username.find(USERNAME_SEPARATOR);
        vmName = username.substr(pos + 1);
        username = username.substr(0, pos);
    }

    void splitUsernameWithToken(std::string& username, std::string& vmId, std::string& token) {
        size_t pos = username.find(USERNAME_SEPARATOR);
        vmId = username.substr(pos + 1);
        username = username.substr(0, pos);

        pos = vmId.find(USERNAME_SEPARATOR);
        token = vmId.substr(pos + 1);
        vmId = vmId.substr(0, pos);
    }
};