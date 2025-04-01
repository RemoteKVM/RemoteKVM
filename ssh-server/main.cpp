#include <iostream>
#include "SSHServer.h"

#define PORT
int main() {
    try {
        // Create and start the SSH server on port 7777
        SSHServer server;
        server.start_server(7777);

    }
    catch (const std::exception& e) 
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
