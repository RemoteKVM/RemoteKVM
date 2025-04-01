#pragma once

#include <vector>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include "SSHClientThread.h"


class SSHServer {
private:
    int server_socket;
    std::vector<std::thread> client_threads;
    std::mutex client_mutex;
    const int max_clients = 10; // TODO: think how meny clinets we can allow to hendle at once.
    
public:
    SSHServer();
    ~SSHServer();
    void start_server(int port);
    void accept_clients();
};

