#include "SSHServer.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <functional> // for std::bind


SSHServer::SSHServer() : server_socket(-1) {}

SSHServer::~SSHServer() {
    if (server_socket != -1) 
    {
        close(server_socket);
    }
    // Join all threads to ensure they finish execution
    for (auto& thread : client_threads) 
    {
        if (thread.joinable()) 
        {
            thread.join();
        }
    }
}

void SSHServer::start_server(int port) {
    struct sockaddr_in server_addr;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) 
    {
        std::cerr << "Failed to create socket!" << std::endl;
        return;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        std::cerr << "Failed to bind socket!" << std::endl;
        close(server_socket);
        return;
    }

    if (listen(server_socket, 5) == -1) 
    {
        std::cerr << "Failed to listen on socket!" << std::endl;
        close(server_socket);
        return;
    }

    std::cout << "SSH Server started on port " << port << std::endl;
    accept_clients();
}

void SSHServer::accept_clients() {
    while (true) 
    {
        int client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) 
        {
            perror("Accept failed");
            continue;
        }
        std::cout << "New client connected!" << std::endl;
        
        SSHClientThread* clientThread = new SSHClientThread(client_socket);
        
        std::thread t(&SSHClientThread::handle_client, clientThread);
        t.detach();
    }
}