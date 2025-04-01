#pragma once

#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
#include "SubprocessHandler.h"
#include "KeyExchangeHandler.h"
#include "AuthenticationHandler.h"
#include "ChannelsHandler.h"
#include "SshSocket.h"
#include "VmsHandler.h"

#define CLIENT_DISCONNECT_CHAR "\x03"
class SSHClientThread {
public:
    explicit SSHClientThread(int socket);
    ~SSHClientThread();
    void handle_client();

private:
    int client_socket;
    SshSocket ssh_socket;
    std::thread client_thread;    
    std::string vmId;

    DbHandler dbHandler; // Note: must be declared before the vms_handler
    KeyExchangeHandler kex_hendler;
    AuthenticationHandler auth_handler;
    ChannelsHandler channels_handler;
    VmsHandler vms_handler;


    void send_logo_ascii_art();
};

