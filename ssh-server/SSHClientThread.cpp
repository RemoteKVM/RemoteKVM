#include "SSHClientThread.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm> // for std::max
#include <string.h>

SSHClientThread::SSHClientThread(int socket) : client_socket(socket), ssh_socket(socket), kex_hendler(client_socket, &ssh_socket), auth_handler(&ssh_socket, &vms_handler, &dbHandler), channels_handler(&ssh_socket), vms_handler(&channels_handler, &dbHandler, socket) {}

SSHClientThread::~SSHClientThread() {
    try
    {
        ssh_socket.sendDisconnectMessage();
    }
    catch(const std::exception& e)
    {
        std::cerr << "unable to send an disconnect msg\n";
    }    
    
    std::cout << "Closing client socket" << std::endl;
    close(client_socket);
}

void SSHClientThread::send_logo_ascii_art() {
    std::string asciiArt = // the text is as ascii art: "RemoteKVM\nby liron and omer" and under it "Welcome to remoteKVM!" with colors.
    "\033[38;5;43m       ____                               _            \033[38;5;23m_  __ __     __  __  __ \r\n"
    "\033[38;5;43m      |  _ \\    ___   _ __ ___     ___   | |_    ___  \033[38;5;23m| |/ / \\ \\   / / |  \\/  |\r\n"
    "\033[38;5;43m      | |_) |  / _ \\ | '_ ` _ \\   / _ \\  | __|  / _ \\ \033[38;5;23m| ' /   \\ \\ / /  | |\\/| |\r\n"
    "\033[38;5;43m      |  _ <  |  __/ | | | | | | | (_) | | |_  |  __/ \033[38;5;23m| . \\    \\ V /   | |  | |\r\n"
    "\033[38;5;43m      |_| \\_\\  \\___| |_| |_| |_|  \\___/   \\__|  \\___| \033[38;5;23m|_|\\_\\    \\_/    |_|  |_|\r\n"
    "\033[38;5;73m  _          \033[38;5;23m   _     _            \033[38;5;73m                       _   \033[38;5;43m ___                      \r\n"
    "\033[38;5;73m | |__  _   _  \033[38;5;23m| |   (_)_ __ ___  _ __    \033[38;5;73m __ _ _ __   __| | \033[38;5;43m / _ \\ _ __ ___   ___ _ __ \r\n"
    "\033[38;5;73m | '_ \\| | | | \033[38;5;23m| |   | | '__/ _ \\| '_ \\\033[38;5;73m   / _` | '_ \\ / _` | \033[38;5;43m| | | | '_ ` _ \\ / _ \\ '__|\r\n"
    "\033[38;5;73m | |_) | |_| | \033[38;5;23m| |___| | | | (_) | | | |\033[38;5;73m | (_| | | | | (_| | \033[38;5;43m| |_| | | | | | |  __/ |   \r\n"
    "\033[38;5;73m |_.__/ \\__, | \033[38;5;23m|_____|_|_|  \\___/|_| |_|\033[38;5;73m  \\__,_|_| |_|\\__,_|  \033[38;5;43m\\___/|_| |_| |_|\\___|_|   \r\n"
    "\033[38;5;73m        |___/                                                                            \r\n"
    "\r\n\033[0m"
    "\033[4mWelcome to \033[1;38;5;36mremote\033[38;5;23mKVM\033[0m\033[4m!\033[0m\r\n";

    channels_handler.sendDataOnChannel(asciiArt);
}

void SSHClientThread::handle_client() {
    try
    {
        kex_hendler.perform_ssh_init();
        auth_handler.perform_authentication();
        channels_handler.InitChannels();
        send_logo_ascii_art();
        vmId = vms_handler.handleVmSelection();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Error in setting up the ssh connection: " << e.what() << '\n';
        this->~SSHClientThread();
        return;
    }
        
    SubprocessHandler subprocess_handler = SubprocessHandler(vmId);
    
    fd_set read_fds;
    char buffer[1024];

    // Determine the maximum descriptor for select()
    int max_fd = std::max(client_socket, subprocess_handler.out_pipe[0]);
    
    while (true) 
    {
        FD_ZERO(&read_fds);
        FD_SET(subprocess_handler.out_pipe[0], &read_fds);   // Monitor hypervisor output
        FD_SET(client_socket, &read_fds); // Monitor client input

        struct timeval timeout = { 1, 0 }; // 1-second timeout
        int ready = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (ready > 0) 
        {
            // If data is available from the hypervisor, read it and send it to the client.
            if (FD_ISSET(subprocess_handler.out_pipe[0], &read_fds)) 
            {
                ssize_t bytes_read = read(subprocess_handler.out_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) 
                {
                    buffer[bytes_read] = '\0';
                    
                    try
                    {
                        channels_handler.sendDataOnChannel(buffer);
                    }
                    catch(const std::exception& e)
                    {
                        std::cerr << "Error in sending data to remote clinet: " << e.what() << '\n';
                        break;
                    }
                }
            }
            // If data is available from the client, read it and forward it to the hypervisor.
            if (FD_ISSET(client_socket, &read_fds)) 
            {
                try
                {
                    std::string client_input = channels_handler.readDataFromChannel();
                    if (client_input.empty()) 
                    {
                        continue; // the packet that was read was protocol packet and not a data packet.
                    }
                    else if (client_input == CLIENT_DISCONNECT_CHAR) 
                    {
                        break;
                    }
                    ssize_t bytes_sent = write(subprocess_handler.in_pipe[1], client_input.c_str(), client_input.size());
                    if (bytes_sent <= 0) 
                    {
                        throw SshException("Error: Failed to send data to hypervisor. Bytes sent: " + bytes_sent);
                    }
                }
                catch(const std::exception& e)
                {
                    std::cerr << "Error in reading from client: " << e.what() << '\n';
                    break;
                }
            }
        } else if (ready < 0) 
        {
            std::cerr << "Error in select(): " << errno << std::endl;
            break;
        }
        // If timeout, loop again.
    }
    
    this->~SSHClientThread();
}