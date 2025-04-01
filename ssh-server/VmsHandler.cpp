#include "VmsHandler.h"

VmsHandler::VmsHandler(ChannelsHandler* channels_handler, DbHandler* dbHandler, int socket) : channels_handler(channels_handler), dbHandler(dbHandler), socket(socket) {}
    
VmsHandler::~VmsHandler() {
    if (selectedVm.id != -1) {
        // Update the VM status to "stopped" if it was running
        dbHandler->updateVmStatus(selectedVm.id, "stopped");
    }
}


void VmsHandler::setVms(std::vector<VirtualMachine> vms)
{
    this->vms = vms;
}

void VmsHandler::setSelectedVm(VirtualMachine selectedVm)
{
    this->selectedVm = selectedVm;
}


std::string VmsHandler::handleVmSelection()
{
    if (selectedVm.id == -1) {
        // If no VM is pre-selected, let the user choose one
        channels_handler->sendDataOnChannel("\r\n\033[4mPlease enter the number of the VM (Virtual Mashine) you what to connect to:\033[0m\r\n");
        for (size_t i = 0; i < vms.size(); i++) {
            std::string color = (i % 2 == 0) ? "\033[38;5;43m" : "\033[38;5;23m";
            channels_handler->sendDataOnChannel(color + std::to_string(i + 1) + ". " + vms[i].name + "\033[0m\r\n");
        }
        channels_handler->sendDataOnChannel("\r\n\033[4mYour choice:\033[0m ");

        int vmIndex;
        try {
            vmIndex = std::stoi(getUserSelection()) - 1;
        }
        catch (const std::exception& e) {
            channels_handler->sendDataOnChannel("\r\n\033[38;5;196mInvalid VM number!\033[0m\r\n");
            return handleVmSelection();
        }

        if (vmIndex < 0 || vmIndex >= vms.size()) {
            channels_handler->sendDataOnChannel("\r\n\033[38;5;196mInvalid VM number!\033[0m\r\n");
            return handleVmSelection();
        }
        if (vms[vmIndex].status == "running") {
            channels_handler->sendDataOnChannel("\r\n\033[38;5;196mYou are Connected to this VM already! Please select another VM or close the connection to this VM first.\033[0m\r\n");
            return handleVmSelection();
        }
        selectedVm = vms[vmIndex];
    }
    if (selectedVm.status == "running") {
        selectedVm.id = -1;
        channels_handler->sendDataOnChannel("\r\n\033[38;5;196mYou are Connected to this VM already! you need to disconnect from it first in order to connect again.\033[0m\r\n");
        usleep(100000);  // 100,000 microseconds = 0.1 seconds
        throw(SshException("VM in running"));
    }

    // Activate the VM storage
    channels_handler->sendDataOnChannel("\r\n\033[38;5;43mActivating the storage of " + selectedVm.name + " VM...\033[0m\r\n");
    activateVmStorage(std::to_string(selectedVm.id), selectedVm.storageSize);
    channels_handler->sendDataOnChannel("\033[38;5;23mStorage activated successfully!\033[0m\r\n\r\n");
    
    // Update the VM status to "running"
    dbHandler->updateVmStatus(selectedVm.id, "running");

    return std::to_string(selectedVm.id);
}

void VmsHandler::activateVmStorage(std::string vmId, int storageSize)
{
    // Define the file path for the ext4 image
    std::string imagePath = "./hypervisor/vmStorage/" + vmId + ".ext4";
    
    // Check if file already exists
    std::ifstream fileCheck(imagePath);
    if (fileCheck.is_open()) {
        // File already exists, close file stream and return
        return;
    }
    fileCheck.close();
        
    // Convert storage size from MB to blocks (4 kb each)
    unsigned int blocksCount = (storageSize * 1024) / 4;
    
    // Create a sparse file of the specified size
    std::string dd_command = "dd if=/dev/zero of=" + imagePath + " bs=4k count=" + std::to_string(blocksCount) + " 2>/dev/null";
    int dd_result = system(dd_command.c_str());
    if (dd_result != 0) {
        throw std::runtime_error("Failed to create VM storage file with dd");
    }
    
    // Format the file as ext4
    std::string mkfs_command = "mkfs.ext4 -F " + imagePath + " 2>/dev/null";
    int mkfs_result = system(mkfs_command.c_str());
    if (mkfs_result != 0) {
        throw std::runtime_error("Failed to format VM storage file as ext4");
    }
}

std::string VmsHandler::getUserSelection()
{
    std::string userResponse = "";
    char buffer[1024];
    
    fd_set read_fds;
    struct timeval timeout;
    
    while (true) {
        // Set up the file descriptor set
        FD_ZERO(&read_fds);
        FD_SET(socket, &read_fds); // Monitor client input
        
        // Set timeout (1 second)
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        // Check if there's data available to read
        int ready = select(socket + 1, &read_fds, nullptr, nullptr, &timeout);
        
        if (ready > 0) {
            // If data is available from the client, read it
            if (FD_ISSET(socket, &read_fds)) {
                try {
                    std::string client_input = channels_handler->readDataFromChannel();
                    
                    // If we received a newline, we're done
                    if (client_input.find('\n') != std::string::npos || client_input.find('\r') != std::string::npos) {
                        break;
                    }
                    
                    channels_handler->sendDataOnChannel(client_input);
                    
                    userResponse += client_input;
                    
                }
                catch (const std::exception& e) {
                    std::cerr << "Error in reading from client: " << e.what() << '\n';
                    throw;
                }
            }
        } else if (ready < 0) {
            // Error in select
            std::cerr << "Error in select(): " << errno << std::endl;
            throw std::runtime_error("Select failed in getUserSelection");
        }
        // If timeout, loop again
    }
    
    return userResponse;
}